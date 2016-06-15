-- grnreaderX Import Script for 3ds Max
--
-- Copyright (c) 2016 Cliff Kang
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
--
--

-- import structure definitions

struct gmodel (
	name,
	skeleton,
	meshes
)

struct gskeleton (
	bones
)

struct gbone (
	name,
	parent_index,
	transform
)

struct gmesh (
	name,
	vertices,
	bones,
	material_indices,
	face_group
)

struct gvertex (
	position,
	normal,
	uv,
	bone_indices,
	weights
)

struct gface_group (
	mat_id,
	faces
)

struct gmaterial (
	name,
	diffuse,
	opacity,
	bump
)

struct ganimation (
	frame_count,
	fps,
	controller_groups
)

struct gcontroller_group (
	name,
	controllers
)

struct gcontroller (
	name,
	position_keys,
	rotation_keys,
	scale_keys
)

struct gkey (
	frame,
	value
)

-- global variables
importer_title = "grnreaderX"
importer_bone_size = 0.05

struct gimporter (
	-- import data
	materials,
	models,
	animations,
	
	scene_skeletons,
	scene_materials,
	
	imp_mat_id,
	imp_mat,
	imp_model_id,
	imp_model,
	imp_mesh_id,
	imp_mesh,
	imp_vertex_id,
	imp_vertex,
	imp_face_group_id,
	imp_faces,
	imp_face_id,
	imp_anim_id,
	imp_anim,
	imp_ctrl_group_id,
	imp_ctrl_group,
	imp_ctrl_id,
	imp_ctrl,
	
	-- utility functions
	fn find_bone name objs =
	(
		local result
		for b in objs where (classof b == BoneGeometry or classof b == Point or classof b == Biped_Object or classof b == Dummy) and b.name == name do
		(
			result = b
		)
		result
	),
	
	-- scene creation functions
	fn create_material m = (
		local result = StandardMaterial()
		result.name = m.name
		if m.diffuse != undefined then result.diffuseMap = bitmaptexture filename: m.diffuse
		if m.opacity != undefined then result.opacityMap = bitmaptexture filename: m.opacity
		if m.bump != undefined then result.bumpMap = bitmaptexture filename: m.bump
		result
	),
	
	fn create_bone b si = (
		local result
		local bone_zaxis = [0, 1, 0]
		
		if b.parent_index == 0 then result = BoneSys.createBone b.transform.pos b.transform.pos bone_zaxis
		else result = BoneSys.createBone b.transform.pos scene_skeletons[si].bones[b.parent_index].transform.pos bone_zaxis
		
		result.name = b.name
		
		-- fix bone transform
		local offset_transform = (scaleMatrix result.objectOffsetScale) * (result.objectOffsetRot as matrix3) * (transMatrix result.objectOffsetPos)
		offset_transform *= result.transform * (inverse b.transform)
		result.transform = b.transform
		result.objectOffsetPos = offset_transform.translation
		result.objectOffsetRot = offset_transform.rotation
		result.objectOffsetScale = offset_transform.scale
		
		--fix negative scale
		if result.scale[1] < -0.999999 and result.scale[2] < -0.999999 and result.scale[3] < -0.999999 then result.objectOffsetScale = [-1, -1, -1]
		
		result.width = importer_bone_size
		result.height = importer_bone_size
		
		if b.parent_index > 0 then result.parent = scene_skeletons[si].bones[b.parent_index]
		result
	),
	
	fn create_mesh m si = (
		local face_count = 0
		for face_material_group in m.face_group do face_count = face_count + face_material_group.faces.count
		
		local result = mesh name:m.name numverts:m.vertices.count numfaces:face_count
		setNumTVerts result m.vertices.count
		buildTVFaces result
		for i = 1 to m.vertices.count do 
		(
			local vertex = m.vertices[i]
			setVert result i vertex.position
			setNormal result i vertex.normal
			setTVert result i vertex.uv[1] vertex.uv[2] 0
		)
		
		local parent_material = multiMaterial numSubs: m.material_indices.count
		for i = 1 to m.material_indices.count do parent_material[i] = scene_materials[m.material_indices[i]]
		result.material = parent_material
			
		local face_index = 1
		for face_material_group in m.face_group do
		( 
			for face in face_material_group.faces do
			(
				setFace result face_index face
				setFaceMatID result face_index face_material_group.mat_id
				face_index = face_index + 1
			)
		)
		
		for i = 1 to result.numFaces do setTVFace result i (getface result i)
		
		Select result
		max modify mode	
		
		local mod_edit_normals = Edit_Normals()
		addModifier result mod_edit_normals
		for f = 1 to getNumFaces result do
		(
			local face = getFace result f
			for i = 1 to 3 do
			(
				local normal_id = mod_edit_normals.getNormalID f i
				mod_edit_normals.setNormal normal_id m.vertices[face[i]].normal
				mod_edit_normals.setNormalExplicit normal_id explicit:true
			)
		)
		
		local mod_skin = Skin()
		addModifier result mod_skin
		
		for i in m.bones do
		(
			local b = find_bone i scene_skeletons[si].bones
			if b != undefined then skinOps.addBone mod_skin b 1
		)
		disableSceneRedraw()
		
		if m.vertices[1].weights != undefined then
		(
			for i = 1 to result.numVerts do
			(
				local vertex_weights = m.vertices[i].weights
				local bone_indices = m.vertices[i].bone_indices
				if vertex_weights[1] == 1 then
				(
					skinOps.ReplaceVertexWeights mod_skin i bone_indices[1] 1.0
				)
				else
				(
					local indices = #()
					local weights = #()
					for w = 1 to 4 do
					(
						if vertex_weights[w] > 0 then
						(
							append indices bone_indices[w]
							append weights vertex_weights[w]
						)
					)
					
					skinOps.ReplaceVertexWeights mod_skin i indices weights
				)
			)
		)
		enableSceneRedraw()
		result
	),	
	
	fn create_model i = (
		local model = models[i]
		local result
		local objs = #()
		
		scene_skeletons[i] = gskeleton #()
		for b in model.skeleton.bones do
		(
			local scene_bone = create_bone b i
			append scene_skeletons[i].bones scene_bone
			append objs scene_bone
			
		)
		
		for m in model.meshes do append objs (create_mesh m i)
		
		result = group objs
		result.name = model.name
	),
	
	fn create_animation a = (
		frameRate = a.fps
		animationRange = interval 0f a.frame_count
		for g in a.controller_groups do
		(
			local objs = objects
			for m = 1 to models.count do 
			(
				if g.name == models[m].name then objs = scene_skeletons[m].bones
			)
			for c in g.controllers do
			(
				local b = find_bone c.name objs
				if b != undefined then
				(
					if c.position_keys.count > 0 then
					(
						local pos_ctrl = bezier_position()
						b.pos.controller = pos_ctrl
						
						for k in c.position_keys do
						(
							local pos_key = addNewKey pos_ctrl k.frame
							pos_key.inTangentType = #slow
							pos_key.outTangentType = #slow
							pos_key.value = k.value
						)
					)
					
					if c.rotation_keys.count > 0 then
					(
						local rot_ctrl = bezier_rotation()
						b.rotation.controller = rot_ctrl
						
						for k in c.rotation_keys do
						(
							local rot_key = addNewKey rot_ctrl k.frame
							rot_key.value = k.value
						)
					)

					if c.scale_keys.count > 0 then
					(
						local scale_ctrl = bezier_scale()
						b.scale.controller = scale_ctrl
						
						for k in c.scale_keys do
						(
							local scale_key = addNewKey scale_ctrl k.frame
							scale_key.inTangentType = #slow
							scale_key.outTangentType = #slow					
							scale_key.value = k.value
						)
					)
				)		
			)
		)
	),

	fn clean_up = (
		models = #()
		materials = #()
		animations = #()
		imp_model_id = 0
		imp_mat_id = 0
		imp_anim_id = 0
		
		scene_skeletons = #()
		scene_materials = #()
	),
	
	fn init = (
		clean_up()
	),
	
	fn import = (	
		local ret = true
		local group_objs = #()
		
		try
		(
			local mat_id = 0
			local meditmat_id = 1
			local mat_imported = 0
			for i = 1 to materials.count do
			(
				append scene_materials (create_material materials[i])
				local imported = false
				for m = meditmat_id to 24 do
				(
					if not imported then
					(
						if classof meditmaterials[m] == StandardMaterial then
						(
							if meditmaterials[m].diffuseMap == undefined and meditmaterials[m].opacityMap == undefined and meditmaterials[m].bumpMap == undefined then
							(
								mat_id += 1
								meditmaterials[m] = scene_materials[mat_id]
								meditmaterials[m].showInViewport = on
								scene_materials[mat_id] = meditmaterials[m]
								mmeditmat_id = m + 1
								imported = true
								mat_imported += 1
							)
						)
					)
				)
			)
			if mat_imported < scene_materials.count then
			(
				messageBox (((scene_materials.count - mat_imported) as string) + " materials of the cannot be imported to Material Editor") title:importer_title 
			)
			
			for i = 1 to models.count do create_model i
			
			for a in animations do create_animation a

			clean_up()
		)
		catch
		(
			messageBox "Import Failed!" title:importer_title
			ret = false
		)
	)
	
)

--- importer initialization ---
importer = gimporter()
importer.init()

-- file import function definitions

-- material
fn mat name = (
	importer.imp_mat_id += 1
	append importer.materials (gmaterial name)
	importer.imp_mat = importer.materials[importer.imp_mat_id]
)

-- diffuse texture
fn td name = (
	importer.imp_mat.diffuse = name
)

-- opacity texture
fn tp name = (
	importer.imp_mat.opacity = name
)

-- bump texture
fn tb name = (
	importer.imp_mat.bump = name
)

-- model
fn md name = (
	importer.imp_model_id += 1
	append importer.models (gmodel name (gskeleton #()) #())
	importer.imp_model = importer.models[importer.imp_model_id]
	importer.imp_mesh_id = 0
)

-- bone
fn b name parent_index tm1 tm2 tm3 tm4 tm5 tm6 tm7 tm8 tm9 tm10 tm11 tm12 = (
	append importer.imp_model.skeleton.bones (gbone name parent_index (matrix3 [tm1, tm2, tm3] [tm4, tm5, tm6] [tm7, tm8, tm9] [tm10, tm11, tm12]))
)

-- mesh
fn m name = (
	importer.imp_mesh_id += 1
	append importer.imp_model.meshes (gmesh name #() #() #() #()#())
	importer.imp_mesh = importer.imp_model.meshes[importer.imp_mesh_id]
	importer.imp_vertex_id = 0
	importer.imp_face_group_id = 0
)

-- mesh bone
fn mb name = (
	append importer.imp_mesh.bones name
)

-- mesh material 
fn mm i = (
	append importer.imp_mesh.material_indices i
)

-- vertex position
fn v x y z = (
	importer.imp_vertex_id += 1
	append importer.imp_mesh.vertices (gvertex [x,y,z])
	importer.imp_vertex = importer.imp_mesh.vertices[importer.imp_vertex_id]
)

-- vertex normal
fn vn x y z = (
	importer.imp_vertex.normal = [x,y,z]
)

-- vertex uv
fn vt x y = (
	importer.imp_vertex.uv = [x,y]
)

-- vertex weights
fn vw w1 w2 w3 w4 i1 i2 i3 i4 = (
	importer.imp_vertex.weights = #(w1, w2, w3, w4)
	importer.imp_vertex.bone_indices = #(i1, i2, i3, i4)
)

-- face group
fn fg i = (
	importer.imp_face_group_id += 1
	append importer.imp_mesh.face_group (gface_group i #())
	importer.imp_faces = importer.imp_mesh.face_group[importer.imp_face_group_id].faces
)

-- face
fn f i1 i2 i3 = (
	append importer.imp_faces [i1, i2, i3]
)

-- animation
fn a frame_count fps = (
	importer.imp_anim_id += 1
	append importer.animations (ganimation frame_count fps #())
	importer.imp_anim = importer.animations[importer.imp_anim_id]
	importer.imp_ctrl_group_id = 0
)

-- controller group
fn cg name = (
	importer.imp_ctrl_group_id += 1
	append importer.imp_anim.controller_groups (gcontroller_group name #())
	importer.imp_ctrl_group = importer.imp_anim.controller_groups[importer.imp_ctrl_group_id]
	importer.imp_ctrl_id = 0
)

-- controller
fn c name = (
	importer.imp_ctrl_id += 1
	append importer.imp_ctrl_group.controllers (gcontroller name #() #() #())
	importer.imp_ctrl = importer.imp_ctrl_group.controllers[importer.imp_ctrl_id]
)

-- key position
fn kp i x y z = (
	append importer.imp_ctrl.position_keys (gkey i [x,y,z])
)

-- key rotation
fn kr i x y z w = (
	append importer.imp_ctrl.rotation_keys (gkey i (quat x y z w))
)

-- key scale
fn ks i x y z = (
	append importer.imp_ctrl.scale_keys (gkey i [x,y,z])
)

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
	face_groups
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
	diffuse_id,
	opacity_id,
	bump_id
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
gx_title = "grnreaderX"
if gx_reselect_textures == undefined then gx_reselect_textures = false
if gx_bone_size == undefined then gx_bone_size = [0.05, 0.05]
if gx_slider_time == undefined then gx_slider_time = false
gx_plugin_mode = doesFileExist ((getDir #startupScripts) + "\\grnreaderX.ms")

struct gimporter (
	-- import data
	textures,
	materials,
	models,
	animations,
	
	scene_textures,
	scene_materials,
	scene_skeletons,
	
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
		for b in objs where (classof b == BoneGeometry or classof b == Point or classof b == Biped_Object or classof b == Dummy) and stricmp b.name name == 0 do result = b
		result
	),
	
	-- scene creation functions
	
	fn create_texture t = (
		bitmaptexture filename:t
	),
	
	fn create_material m = (
		local result = StandardMaterial()
		result.name = m.name
		if m.diffuse_id != undefined then result.diffuseMap = scene_textures[m.diffuse_id]
		if m.opacity_id != undefined then result.opacityMap = scene_textures[m.opacity_id]
		if m.bump_id != undefined then result.bumpMap = scene_textures[m.bump_id]
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
		
		result.width = gx_bone_size.x
		result.height = gx_bone_size.y
		
		if b.parent_index > 0 then result.parent = scene_skeletons[si].bones[b.parent_index]
		result
	),
	
	fn create_mesh m si = (
		local face_count = 0
		for g in m.face_groups do face_count = face_count + g.faces.count
		
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
		for g in m.face_groups do
		( 
			for face in g.faces do
			(
				setFace result face_index face
				setFaceMatID result face_index g.mat_id
				face_index += 1
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
		
		for b in m.bones do skinOps.addBone mod_skin (find_bone b scene_skeletons[si].bones) 1
		
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
		local start = 0f
		local end = a.frame_count as time
		frameRate = a.fps
		if gx_slider_time then
		(
			start = sliderTime
			end += sliderTime
		)
		animationRange = interval 0f end
		for g in a.controller_groups do
		(
			local objs = objects
			local msg
			for m = 1 to models.count do 
			(
				if stricmp g.name models[m].name == 0 then objs = scene_skeletons[m].bones
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
							local pos_key = addNewKey pos_ctrl (start + k.frame)
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
							local rot_key = addNewKey rot_ctrl (start + k.frame)
							rot_key.value = k.value
						)
					)

					if c.scale_keys.count > 0 then
					(
						local scale_ctrl = bezier_scale()
						b.scale.controller = scale_ctrl
						
						for k in c.scale_keys do
						(
							local scale_key = addNewKey scale_ctrl (start + k.frame)
							scale_key.inTangentType = #slow
							scale_key.outTangentType = #slow					
							scale_key.value = k.value
						)
					)
				)
				else
				(
					if msg == undefined then msg = "create_animation error - the following bones in controller group '" + g.name + "' cannot be found:\n\n"
					msg += c.name
					msg += "\n"
				)
			)
			if msg != undefined then messageBox msg title:gx_title
		)
	),

	fn clean_up = (
		textures = #()
		materials = #()
		models = #()
		animations = #()
		
		scene_textures = #()
		scene_materials = #()
		scene_skeletons = #()
		
		imp_mat_id = 0
		imp_model_id = 0
		imp_anim_id = 0
		
		imp_mat = undefined
		imp_model = undefined
		imp_mesh = undefined
		imp_vertex = undefined
		imp_faces = undefined
		imp_anim = undefined
		imp_ctrl_group = undefined
		imp_ctrl = undefined
	),
	
	fn init = (
		clean_up()
	),
	
	fn import = (	
		try
		(
			local group_objs = #()
			local ver = maxVersion()
			
			-- use slate material editor if available
			local sme_avl = ver[1] >= 13000
			
			if gx_reselect_textures then
			(
				for t = 1 to textures.count do
				(
					local file = getOpenFileName filename:textures[t] types:"Targa image file(*.tga)|*.tga|Adobe psd reader(*.psd)|*.psd|"
					if file != undefined then textures[t] = file
				)
			)
			
			for t in textures do append scene_textures (create_texture t)
			
			if materials.count > 0 then
			(
				if not sme_avl then
				(
					local msg		
					local mat_id = 0
					local meditmat_id = 1
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
									)
								)
							)
						)
						if not imported then
						(
							if msg == undefined then msg = "import warning - the following materials cannot be imported because there are no remaining slots available in Material Editor:\n\n"
							msg += materials[i].name
							msg += "\n"
						)
					)
					if msg != undefined then messageBox msg title:gx_title 
				)
				else
				(
					local view_name
					local model_name = ""
					local sme_view
					local view_id = 1
					
					if models.count > 0 then model_name = " - " + models[1].name
					
					sme.Open()
					view_name = "View" + (view_id as string) + model_name
					while sme.GetViewByName view_name > 0 do
					(
						view_id += 1
						view_name = "View" + (view_id as string) + model_name
					)
					
					sme_view = sme.GetView (sme.CreateView view_name)
					for i = 1 to materials.count do 
					(
						append scene_materials (create_material materials[i])
						sme_view.createNode scene_materials[i] [0, 300 * (i - 1)]
						scene_materials[i].showInViewport = on
					)
					sme.Close()
				)
			)
			
			for i = 1 to models.count do create_model i
			
			for a in animations do create_animation a

			clean_up()
		)
		catch
		(
			messageBox "import failed - script file is corrupted." title:gx_title
		)
	)
	
)

--- importer initialization ---
gx_importer = gimporter()
gx_importer.init()

-- file import function definitions

-- texture
fn t name = (
	append gx_importer.textures name
)

-- material
fn mat name = (
	gx_importer.imp_mat_id += 1
	append gx_importer.materials (gmaterial name)
	gx_importer.imp_mat = gx_importer.materials[gx_importer.imp_mat_id]
)

-- diffuse texture
fn td id = (
	gx_importer.imp_mat.diffuse_id = id
)

-- opacity texture
fn tp id = (
	gx_importer.imp_mat.opacity_id = id
)

-- bump texture
fn tb id = (
	gx_importer.imp_mat.bump_id = id
)

-- model
fn md name = (
	gx_importer.imp_model_id += 1
	append gx_importer.models (gmodel name (gskeleton #()) #())
	gx_importer.imp_model = gx_importer.models[gx_importer.imp_model_id]
	gx_importer.imp_mesh_id = 0
)

-- bone
fn b name parent_index tm1 tm2 tm3 tm4 tm5 tm6 tm7 tm8 tm9 tm10 tm11 tm12 = (
	append gx_importer.imp_model.skeleton.bones (gbone name parent_index (matrix3 [tm1, tm2, tm3] [tm4, tm5, tm6] [tm7, tm8, tm9] [tm10, tm11, tm12]))
)

-- mesh
fn m name = (
	gx_importer.imp_mesh_id += 1
	append gx_importer.imp_model.meshes (gmesh name #() #() #() #())
	gx_importer.imp_mesh = gx_importer.imp_model.meshes[gx_importer.imp_mesh_id]
	gx_importer.imp_vertex_id = 0
	gx_importer.imp_face_group_id = 0
)

-- mesh bone
fn mb name = (
	append gx_importer.imp_mesh.bones name
)

-- mesh material 
fn mm i = (
	append gx_importer.imp_mesh.material_indices i
)

-- vertex position
fn v x y z = (
	gx_importer.imp_vertex_id += 1
	append gx_importer.imp_mesh.vertices (gvertex [x,y,z])
	gx_importer.imp_vertex = gx_importer.imp_mesh.vertices[gx_importer.imp_vertex_id]
)

-- vertex normal
fn vn x y z = (
	gx_importer.imp_vertex.normal = [x,y,z]
)

-- vertex uv
fn vt x y = (
	gx_importer.imp_vertex.uv = [x,y]
)

-- vertex weights
fn vw w1 w2 w3 w4 i1 i2 i3 i4 = (
	gx_importer.imp_vertex.weights = #(w1, w2, w3, w4)
	gx_importer.imp_vertex.bone_indices = #(i1, i2, i3, i4)
)

-- face group
fn fg i = (
	gx_importer.imp_face_group_id += 1
	append gx_importer.imp_mesh.face_groups (gface_group i #())
	gx_importer.imp_faces = gx_importer.imp_mesh.face_groups[gx_importer.imp_face_group_id].faces
)

-- face
fn f i1 i2 i3 = (
	append gx_importer.imp_faces [i1, i2, i3]
)

-- animation
fn a frame_count fps = (
	gx_importer.imp_anim_id += 1
	append gx_importer.animations (ganimation frame_count fps #())
	gx_importer.imp_anim = gx_importer.animations[gx_importer.imp_anim_id]
	gx_importer.imp_ctrl_group_id = 0
)

-- controller group
fn cg name = (
	gx_importer.imp_ctrl_group_id += 1
	append gx_importer.imp_anim.controller_groups (gcontroller_group name #())
	gx_importer.imp_ctrl_group = gx_importer.imp_anim.controller_groups[gx_importer.imp_ctrl_group_id]
	gx_importer.imp_ctrl_id = 0
)

-- controller
fn c name = (
	gx_importer.imp_ctrl_id += 1
	append gx_importer.imp_ctrl_group.controllers (gcontroller name #() #() #())
	gx_importer.imp_ctrl = gx_importer.imp_ctrl_group.controllers[gx_importer.imp_ctrl_id]
)

-- key position
fn kp i x y z = (
	append gx_importer.imp_ctrl.position_keys (gkey i [x,y,z])
)

-- key rotation
fn kr i x y z w = (
	append gx_importer.imp_ctrl.rotation_keys (gkey i (quat x y z w))
)

-- key scale
fn ks i x y z = (
	append gx_importer.imp_ctrl.scale_keys (gkey i [x,y,z])
)

-- importer utility
utility gx_importer_utility "grnreaderX"
(
	local exe = getINISetting ((getDir #plugcfg) + "\\grnreaderX.ini") "grnreaderX" "path"
	
	rollout gx_importer_ui "grnreaderX"
	(
		checkbox cb_reselect_textures "Re-select textures" checked:gx_reselect_textures
		spinner s_bone_width "Bone Width:" range:[gx_bone_size.x, 10000, 0.01] align:#left
		spinner s_bone_height "Bone Height:" range:[gx_bone_size.y, 10000, 0.01] align:#left
		checkbox cb_slider_time "Start from slider" checked:gx_slider_time
		local files = #()
		local items = #()
		group "Files:"
		(
			listbox lb_files ""
			button btn_add "Add" width:60 across:2
			button btn_remove "Remove" width:60
		)
		button btn_import "Import" width:60
		button btn_setup "Setup" width:60
		
		on cb_reselect_textures changed stat do gx_reselect_textures = stat
		
		on s_bone_width changed val do gx_bone_size.x = val
		
		on s_bone_height changed val do gx_bone_size.y = val
		
		on cb_slider_time changed stat do gx_slider_time = stat
		
		on btn_add pressed do
		(
			local file = getOpenFileName types:"Granny Runtime 2 File (*.gr2)|*.gr2|"
			if file != undefined then
			(
				append files file
				append items (filenameFromPath file)
				lb_files.items = items
				lb_files.selection = items.count
			)
		)
		
		on btn_remove pressed do
		(
			if lb_files.selection > 0 then
			(
				local i = lb_files.selection
				deleteItem files i
				deleteItem items i
				lb_files.items = items
			)
		)
		
		on btn_import pressed do
		(
			if files.count > 0 then
			(
				local command = "\" \"" + exe + "\" \"-d\""
				local ver = maxVersion()
				local ret
				
				for f in files do command += " \"" + f + "\""
				command += " \""
				
				if ver[1] >= 12000 then HiddenDOSCommand command ExitCode:&ret
				else ret = DOSCommand command
				
				if ret == 0 then
				(
					local script = (getFilenamePath files[1]) + (getFilenameFile files[1]) + ".ms"
					try
					(
						filein script
					)
					catch
					(
						messageBox "import failed: script file corrupted." title:gx_title
					)
					deleteFile script
					gx_importer.import()
		
					files = #()
					items = #()
					lb_files.items = #()
				)
				else messageBox "import failed: cannot execute external command." title:gx_title
			)
		)
		
		on btn_setup pressed do
		(
			local file = getOpenFileName caption:"Locate 'grnreaderX.exe'" types:"Executeable file(*.exe)|*.exe|"
			if file != undefined then
			(
				if not (setINISetting ((getDir #plugcfg) + "\\grnreaderX.ini") "grnreaderX" "path" file) then
				(
					messageBox "plugin setup: unable to save plugin settings." title:gx_title
				)
				else exe = file
			)			
		)
	)
	
	on gx_importer_utility open do
	(
		if gx_plugin_mode then addRollout gx_importer_ui
	)
	
	on gx_importer_utility close do
	(
		if gx_plugin_mode then removeRollout gx_importer_ui
	)
)

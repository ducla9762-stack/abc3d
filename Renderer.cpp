#include <algorithm>

#include "renderer.h"

// ---------------------------------------------------------------------------------------------
// Core ----------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

bool ABC3DRenderer::Initialize(GLuint defaultFrameBuffer) {

	glGenVertexArrays(2, &vao.mesh);
	bufs.Initialize();

	// Create textures
	glGenTextures(3, &tex.geom);

	std::string header = ConstructShaderHeader(maxObjectCount);

	// Compile shaders
	bool success = prog.mesh.BuildFiles("assets/shaders/GeomPass.vert", "assets/shaders/GeomPass.frag",
		nullptr, nullptr, nullptr, header.data(), &std::cout);
	if (!success) {
		std::cout << "Failed to compile mesh shader program!" << std::endl;
		return false;
	}

	prog.mesh.RegisterUniform(VIEW, "view");

	success = prog.shade.BuildFiles("assets/shaders/BasicLightPass.vert", "assets/shaders/BasicLightPass.frag");
	if (!success) {
		std::cout << "Failed to compile deferred shader program!" << std::endl;
		return false;
	}

	prog.shade.Bind();

	prog.shade.SetUniform("posTex", TexBindings::POS);
	prog.shade.SetUniform("normTex", TexBindings::NORM);
	prog.shade.SetUniform("idTex", TexBindings::ID);

	prog.shade.RegisterUniform(BaseColors::ALL, "colors");
	prog.shade.RegisterUniform(BaseColors::BACKGROUND, "colors[0]");
	prog.shade.RegisterUniform(BaseColors::FACES, "colors[1]");
	prog.shade.RegisterUniform(BaseColors::EDGES, "colors[2]");
	prog.shade.RegisterUniform(BaseColors::VERTICES, "colors[3]");

	prog.shade.SetUniform3(BaseColors::ALL, &colors.background.x, 4);

	prog.shade.RegisterUniform(BlendColors::ALL + 5, "blendClrs");
	prog.shade.RegisterUniform(BlendColors::SELECT + 5, "blendClrs[0]");
	prog.shade.RegisterUniform(BlendColors::HOVER + 5, "blendClrs[1]");
	prog.shade.RegisterUniform(BlendColors::BOTH + 5, "blendClrs[2]");

	prog.shade.SetUniform3(BlendColors::ALL + 5, &blends.select.x, 3);

	rndrs.vert.Initialize(header.data());
	rndrs.edge.Initialize(header.data());
	rndrs.grid.Initialize();

	// Initialize gizmo renderer
	if (!gizmoRenderer.Initialize()) {
		return false;
	}
	gizmoRenderer.Clear();

	// The gizmo programs must exist before their camera uniforms are uploaded.
	//UpdateViewUniforms();

	// Set clear color & blend function
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	initialized = true;

	return true;
}

void ABC3DRenderer::Resize(int width, int height, GLuint defaultFrameBuffer) {
	frame.width = width;
	frame.height = height;
	UpdateProjectionUniforms();
	glViewport(0, 0, frame.width, frame.height);
	InitFramebuffer(defaultFrameBuffer);
	update.rerenderGeom = true;
}

void ABC3DRenderer::Draw(GLuint defaultFramebuffer) {

	if (update.renderBufs) { UpdateRenderBuffers(); }
	if (update.uniformBuf) { UpdateUniformBuffer(); }
	if (update.camera) { UpdateViewUniforms(); }
	if (update.selection || update.hover) { 
		ResetColorBuffer();
		UpdateSelection(); 
		UpdateHover(); 
		UploadColorBuffer(); 
	}

	// Had to duplicate this line outside of the conditional to fix visual bugs on NVIDIA GPU.
	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

	if (update.renderBufs || update.uniformBuf || update.camera || update.rerenderGeom) {

		glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

		glClear(GL_DEPTH_BUFFER_BIT);
		float clearClr[3] = { 1.0f, 1.0f, 1.0f };
		uint clearId[3] = { 0, 0, 0 };
		glClearBufferfv(GL_COLOR, 0, clearClr);
		glClearBufferfv(GL_COLOR, 1, clearClr);
		glClearBufferuiv(GL_COLOR, 2, clearId);

		if (show.surfaces) {
			glBindVertexArray(vao.mesh);
			prog.mesh.Bind();
			glDrawArrays(GL_TRIANGLES, 0, meshVertDrawCount);
		}

		if (show.vertices) {
			rndrs.vert.Draw();
		}

		if (show.edges) {
			rndrs.edge.Draw();
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);
	}

	// Had to duplicate this line outside of the conditional to fix visual bugs on NVIDIA GPU.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);

	// Had to move this line outside of the conditional to fix visual bugs on NVIDIA GPU.
	glBlitFramebuffer(0, 0, frame.width, frame.height, 0, 0, frame.width, frame.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);

	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(vao.frame);
	prog.shade.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glEnable(GL_DEPTH_TEST);

	if (show.grid) {
		glEnable(GL_BLEND);

		rndrs.grid.Draw();

		glDisable(GL_BLEND);
	}

	// Gizmos are a screen-space overlay, drawn after the deferred composition and the grid.
	// Picking uses a separate integer buffer.
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	gizmoRenderer.Draw();
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, frame.gizmoPickFbo);
	glViewport(0, 0, frame.width, frame.height);
	const uint clearGizmoId[3] = { 0, 0, 0 };
	glClearBufferuiv(GL_COLOR, 0, clearGizmoId);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	gizmoRenderer.DrawPick();
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);
	glViewport(0, 0, frame.width, frame.height);

	glDepthMask(GL_TRUE);

	update.ClearAll();
}

// ---------------------------------------------------------------------------------------------
// Buffer Updates ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void ABC3DRenderer::InitFramebuffer(GLuint defaultFrameBuffer) {
	// Delete old framebuffer
	glDeleteFramebuffers(1, &frame.fbo);
	glDeleteRenderbuffers(1, &frame.depthBuf);
	glDeleteFramebuffers(1, &frame.gizmoPickFbo);
	glDeleteTextures(1, &frame.gizmoPickTex);
	glDeleteRenderbuffers(1, &frame.gizmoPickDepth);

	std::cout << "FB 1: " << glGetError() << std::endl;

	// Create new framebuffer
	glGenFramebuffers(1, &frame.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

	std::cout << "FB 2: " << glGetError() << std::endl;

	// Allocate textures and render buffer

	glGenRenderbuffers(1, &frame.depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, frame.depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, frame.width, frame.height);

	glActiveTexture(GL_TEXTURE0 + TexBindings::POS);
	glBindTexture(GL_TEXTURE_2D, tex.geom);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, frame.width, frame.height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE0 + TexBindings::NORM);
	glBindTexture(GL_TEXTURE_2D, tex.norm);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, frame.width, frame.height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE0 + TexBindings::ID);
	glBindTexture(GL_TEXTURE_2D, tex.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, frame.width, frame.height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Attach textures and depth buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frame.depthBuf);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.geom, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tex.norm, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, tex.id, 0);

	// Enable all color attachments for drawing
	GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, drawBuffers);

	// Make sure framebuffer is valid
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer is invalid!" << std::endl;
	}

	// A small, independent integer target stores gizmo IDs for mouse hover and
	// press. It is never sampled by the deferred shading pass.
	glGenFramebuffers(1, &frame.gizmoPickFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, frame.gizmoPickFbo);
	glGenTextures(1, &frame.gizmoPickTex);
	glBindTexture(GL_TEXTURE_2D, frame.gizmoPickTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, frame.width, frame.height, 0,
		GL_RGB_INTEGER, GL_UNSIGNED_INT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame.gizmoPickTex, 0);

	glGenRenderbuffers(1, &frame.gizmoPickDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, frame.gizmoPickDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, frame.width, frame.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frame.gizmoPickDepth);
	const GLenum gizmoDrawBuffer = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &gizmoDrawBuffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Gizmo pick framebuffer is invalid!" << std::endl;
	}

	// Creating the pick texture temporarily changes whichever texture unit was
	// active (normally the deferred ID unit). Restore all deferred sampler
	// bindings before the next shading pass.
	glActiveTexture(GL_TEXTURE0 + TexBindings::POS);
	glBindTexture(GL_TEXTURE_2D, tex.geom);
	glActiveTexture(GL_TEXTURE0 + TexBindings::NORM);
	glBindTexture(GL_TEXTURE_2D, tex.norm);
	glActiveTexture(GL_TEXTURE0 + TexBindings::ID);
	glBindTexture(GL_TEXTURE_2D, tex.id);
	glActiveTexture(GL_TEXTURE0);

	// Re-bind default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);

	std::cout << "FB 3: " << glGetError() << std::endl;
}

void ABC3DRenderer::UpdateRenderBuffers() {
	offset.Clear();
	bufBuilder.Clear();
	uint currOffset = 0;

	offset.faces = 0;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		bufBuilder.AddMeshToAttributes(*mesh, i, currOffset);
		offset.meshFaces.push_back(currOffset);
		currOffset += mesh->FaceCount();
	}

	count.faces = currOffset;

	for (std::vector<uint>& list : edgeIdMap) { list.clear(); }
	if (uint(edgeIdMap.size()) < scene->MeshCount()) {
		uint diff = scene->MeshCount() - uint(edgeIdMap.size());
		edgeIdMap.insert(edgeIdMap.end(), diff, std::vector<uint>());
	}
	offset.edges = currOffset;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		uint edgeCount = bufBuilder.AddMeshToEdgeList(*mesh, i, currOffset, edgeIdMap[i]);
		offset.meshEdges.push_back(currOffset);
		currOffset += edgeCount;
	}

	count.edges = currOffset - offset.edges;
	rndrs.edge.SetEdges(bufBuilder.EdgeIds(), bufBuilder.Edges(), GL_DYNAMIC_DRAW);

	offset.verts = currOffset;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		bufBuilder.AddMeshToVertexList(*mesh, i, currOffset);
		offset.meshVerts.push_back(currOffset);
		currOffset += mesh->VertexCount();
	}

	count.verts = currOffset - offset.verts;
	rndrs.vert.SetVertices(bufBuilder.VertexIds(), bufBuilder.VertexPositions(), GL_DYNAMIC_DRAW);

	if (currOffset != uint(clrs.size())) { update.selection = true; update.hover = true;}
	if (scene->MeshCount() > uint(transformBuf.size())) { update.uniformBuf = true; }

	meshVertDrawCount = bufBuilder.Attributes().size();

	glBindVertexArray(vao.mesh);

	glBindBuffer(GL_ARRAY_BUFFER, bufs.meshVBO);
	glBufferData(GL_ARRAY_BUFFER, bufBuilder.Attributes().size() * sizeof(VertexAttributes), bufBuilder.Attributes().data(), GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes), (GLvoid*)offsetof(VertexAttributes, pos));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAttributes), (GLvoid*)offsetof(VertexAttributes, norm));

	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, sizeof(VertexAttributes), (GLvoid*)offsetof(VertexAttributes, ids));
};

void ABC3DRenderer::UpdateUniformBuffer() {
	transformBuf.clear();
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		transformBuf.emplace_back();
		scene->operator[](i)->getTransform(transformBuf.back());
	}

	uint objCount = uint(transformBuf.size());
	bool recompileShaders = false;
	while (objCount > maxObjectCount) { maxObjectCount <<= 1; recompileShaders = true; }
	while (objCount < maxObjectCount >> 1) { maxObjectCount >>= 1; recompileShaders = true; }
	if (recompileShaders) { RecompileShaders(); }

	glBindBuffer(GL_UNIFORM_BUFFER, bufs.matUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CompactTransform140) * maxObjectCount, nullptr, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CompactTransform140) * transformBuf.size(), transformBuf.data());
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ABC3DRenderer::ResetColorBuffer() {
	clrs.clear();

	uint num = count.faces + count.edges + count.verts;

	if (num == 0) {
		clrs.push_back(0);
	}
	else {
		clrs.insert(clrs.end(), num, 0);
	}
}

void ABC3DRenderer::SetObjectBlendColor(uint meshId, uint8_t blendClr) {
	const HalfEdgeMesh* mesh = scene->operator[](meshId);

	uint ofst = offset.meshFaces[meshId];
	std::fill(
		clrs.begin() + ofst,
		clrs.begin() + ofst + mesh->FaceCount(),
		blendClr
	);

	ofst = offset.meshEdges[meshId];
	uint count;
	if (meshId == uint(offset.meshEdges.size() - 1)) {
		count = offset.verts - ofst;
	}
	else { count = offset.meshEdges[meshId + 1] - ofst; }
	std::fill(
		clrs.begin() + ofst,
		clrs.begin() + ofst + count,
		blendClr
	);

	ofst = offset.meshVerts[meshId];
	std::fill(
		clrs.begin() + ofst,
		clrs.begin() + ofst + mesh->VertexCount(),
		blendClr
	);
}

void ABC3DRenderer::SetFaceBlendColor(uint meshId, uint faceId, uint8_t blendClr) {
	clrs[offset.meshFaces[meshId] + faceId] = blendClr;
	// TODO: Set corresponding edge and vertex colors
}

void ABC3DRenderer::SetEdgeBlendColor(uint meshId, uint edgeIndex, uint8_t blendClr) {
	uint edgeId = edgeIdMap[meshId][edgeIndex];
	if (edgeId == MAX_UINT) { return; }

	clrs[offset.meshEdges[meshId] + edgeId] = blendClr;
	// TODO: Set corresponding vertex colors
}

void ABC3DRenderer::UpdateSelection() {
	const std::vector<SelectionItem>& items = selecQuery->getSelectedItems();
	uint itemCount = uint(items.size());

	for (uint i = 0; i < itemCount; ++i) {
		const SelectionItem& item = items[i];
		Element elmType = selecQuery->getActiveElement();
		switch (elmType) {
		case OBJECT:
			SetObjectBlendColor(item.meshId, BlendColors::SELECT);
			break;

		case FACE:
			SetFaceBlendColor(item.meshId, item.componentId, BlendColors::SELECT);
			break;

		case EDGE:
			SetEdgeBlendColor(item.meshId, item.componentId, BlendColors::SELECT);
			break;

		case VERTEX:
			clrs[offset.meshVerts[item.meshId] + item.componentId] = BlendColors::SELECT;
			break;
		}
	}
}

void ABC3DRenderer::UpdateHover() {
	if (hover.mesh == MAX_UINT) { return; }

	Element elmType = selecQuery->getActiveElement();
	const bool selected = elmType == OBJECT
		? selecQuery->hasSelectedMesh(hover.mesh)
		: selecQuery->isSelected(hover.mesh, hover.element);
	if (selected) { return; }

	switch (elmType) {
	case OBJECT:
		SetObjectBlendColor(hover.mesh, BlendColors::HOVER);
		break;

	case FACE:
		SetFaceBlendColor(hover.mesh, hover.element, BlendColors::HOVER);
		break;

	case EDGE:
		SetEdgeBlendColor(hover.mesh, hover.element, BlendColors::HOVER);
		break;

	case VERTEX:
		clrs[offset.meshVerts[hover.mesh] + hover.element] = BlendColors::HOVER;
		break;
	}
}

// ---------------------------------------------------------------------------------------------
// Uniform Updates ----------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void ABC3DRenderer::UpdateProjectionUniforms() {
	transform.proj.SetPerspective(degreesToRadians(cam.fov), float(frame.width) / float(frame.height), 0.1f, 1000.0f);

	float data[16];
	transform.proj.Get(data);

	prog.mesh.Bind();
	prog.mesh.SetUniformMatrix4("proj", data);

	Matrix4f viewProj = transform.proj * transform.view;
	viewProj.Get(data);

	rndrs.grid.SetTransform(data);
	rndrs.edge.SetViewProjection(data);
	rndrs.vert.SetViewProjection(data);

	gizmoRenderer.SetViewProjectionUniform(data, cam.fov, frame.height);
}

void ABC3DRenderer::UpdateViewUniforms() {
	transform.view = cam.type == ORBIT ? cam.orbit.transform() : cam.free.transform();

	float data[16];
	transform.view.Get(data);
	prog.mesh.Bind();
	prog.mesh.SetUniformMatrix4(VIEW, data);

	Matrix4f viewProj = transform.proj * transform.view;
	viewProj.Get(data);

	rndrs.grid.SetTransform(data);
	rndrs.edge.SetViewProjection(data);
	rndrs.vert.SetViewProjection(data);

	gizmoRenderer.SetViewProjectionUniform(data, cam.fov, frame.height);
}


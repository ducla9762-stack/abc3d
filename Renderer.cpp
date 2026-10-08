#include "renderer.h"

// ---------------------------------------------------------------------------------------------
// Core ----------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

bool ABC3DRenderer::Initialize(GLuint defaultFrameBuffer) {

	glGenVertexArrays(2, &vao.mesh);
	bufs.Initialize();

	// Create textures
	glGenTextures(3, &tex.geom);

	// Compile shaders
	bool success = prog.mesh.BuildFiles("assets/shaders/GeomPass.vert", "assets/shaders/GeomPass.frag",
		nullptr, nullptr, nullptr, HEADER, &std::cout);
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

	prog.shade.RegisterUniform(BlendColors::ALL, "blendClrs");
	prog.shade.RegisterUniform(BlendColors::SELECT, "blendClrs[0]");
	prog.shade.RegisterUniform(BlendColors::HOVER, "blendClrs[1]");
	prog.shade.RegisterUniform(BlendColors::BOTH, "blendClrs[2]");

	prog.shade.SetUniform3(BlendColors::ALL, &blends.select.x, 3);

	rndrs.vert.Initialize();
	rndrs.edge.Initialize();
	rndrs.grid.Initialize();

	// Initialize gizmo renderer
	//if (!gizmoRenderer.Initialize()) {
		//return false;
	//}
	// Clear so we don't draw on startup.
	//gizmoRenderer.Clear();

	// The gizmo programs must exist before their camera uniforms are uploaded.
	//UpdateViewUniforms();

	// Set clear color & blend function
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return true;
}

void ABC3DRenderer::Resize(int width, int height, GLuint defaultFrameBuffer) {
	frame.width = width;
	frame.height = height;
	SetPerspectiveAndViewport();
	InitFramebuffer(defaultFrameBuffer);
}

void ABC3DRenderer::Draw(GLuint defaultFramebuffer) {

	if (update.renderBufs) { UpdateRenderBuffers(); update.renderBufs = false; }
	if (update.camera) { UpdateViewUniforms(); update.camera = false; }

	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

	glClear(GL_DEPTH_BUFFER_BIT);
	float clearClr[3] = { 0.0f, 0.0f, 0.0f };
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

	// Draw gizmos before checking the selection texture.
	/*glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	//arrowRenderer.Draw();
	gizmoRenderer.Draw();
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);*/

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlitFramebuffer(0, 0, frame.width, frame.height, 0, 0, frame.width, frame.height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	glDepthMask(GL_FALSE);

	glBindVertexArray(vao.frame);
	prog.shade.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (show.grid) {
		glEnable(GL_BLEND);

		rndrs.grid.Draw();

		glDisable(GL_BLEND);
	}

	glDepthMask(GL_TRUE);
}

// ---------------------------------------------------------------------------------------------
// Buffer Updates ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void ABC3DRenderer::InitFramebuffer(GLuint defaultFrameBuffer) {
	// Delete old framebuffer
	glDeleteFramebuffers(1, &frame.fbo);
	glDeleteRenderbuffers(1, &frame.depthBuf);

	// Create new framebuffer
	glGenFramebuffers(1, &frame.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, frame.fbo);

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

	// Re-bind default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFrameBuffer);
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

	offset.edges = currOffset;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		uint edgeCount = bufBuilder.AddMeshToEdgeList(*mesh, i, currOffset);
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

	glBindBuffer(GL_UNIFORM_BUFFER, bufs.matUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(CompactTransform140) * transformBuf.size(), transformBuf.data(), GL_DYNAMIC_DRAW);
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

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufs.clrSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, clrs.size() * sizeof(uint), clrs.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// ---------------------------------------------------------------------------------------------
// Uniform and Viewport Updates ----------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void ABC3DRenderer::SetPerspectiveAndViewport() {
	transform.proj.SetPerspective(degreesToRadians(cam.fov), float(frame.width) / float(frame.height), 0.1f, 1000.0f);
	UpdateProjectionUniforms();

	glViewport(0, 0, frame.width, frame.height);
}

void ABC3DRenderer::UpdateProjectionUniforms() {
	float data[16];
	transform.proj.Get(data);

	prog.mesh.Bind();
	prog.mesh.SetUniformMatrix4("proj", data);

	Matrix4f viewProj = transform.proj * transform.view;
	viewProj.Get(data);

	rndrs.grid.SetTransform(data);
	rndrs.edge.SetViewProjection(data);
	rndrs.vert.SetViewProjection(data);

	//arrowRenderer.SetViewProjectionUniform(data, fov, height);
	//gizmoRenderer.SetViewProjectionUniform(data, fov, height);
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

	//arrowRenderer.SetViewProjectionUniform(data, fov, height);
	//gizmoRenderer.SetViewProjectionUniform(data, fov, height);
}


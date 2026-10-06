#include "renderer.h"

void ABC3DRenderer::UpdateProjectionUniforms() {
	float data[16];
	transform.proj.Get(data);

	prog.mesh.Bind();
	prog.mesh.SetUniformMatrix4("proj", data);

	//Matrix4f viewProj = transform.proj * transform.view;
	//viewProj.Get(data);

	//rndrs.grid.SetTransform(data);
	//rndrs.edge.SetViewProjection(data);
	//rndrs.vert.SetViewProjection(data);

	//arrowRenderer.SetViewProjectionUniform(data, fov, height);
	//gizmoRenderer.SetViewProjectionUniform(data, fov, height);
}

void ABC3DRenderer::UpdateRenderBuffers() {
	offset.Clear();
	bufBuilder.Clear();
	uint currOffset = 0;

	offset.faces = 0;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		bufBuilder.AddMeshToAttributes(*mesh, i, currOffset);
		offset.meshFaces.push_back(mesh->FaceCount());
		currOffset += offset.meshFaces.back();
	}

	count.faces = currOffset;

	offset.edges = currOffset;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		uint edgeCount = bufBuilder.AddMeshToEdgeList(*mesh, i, currOffset);
		offset.meshEdges.push_back(edgeCount);
		currOffset += offset.meshEdges.back();
	}

	count.edges = currOffset - offset.edges;
	rndrs.edge.SetEdges(bufBuilder.EdgeIds(), bufBuilder.Edges(), GL_DYNAMIC_DRAW);

	offset.verts = currOffset;
	for (uint i = 0; i < scene->MeshCount(); ++i) {
		const HalfEdgeMesh* mesh = scene->operator[](i);
		bufBuilder.AddMeshToVertexList(*mesh, i, currOffset);
		offset.meshVerts.push_back(mesh->VertexCount());
		currOffset += offset.meshVerts.back();
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
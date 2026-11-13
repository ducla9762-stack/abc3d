#ifndef _ABC3D_RENDER_2_0_H_
#define _ABC3D_RENDER_2_0_H_

#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <chrono>
#include <string>

#include <cyGL.h>
#include <cyMatrix.h>

#include "helper.h"
#include "selectionManager.h"
#include "scene.h"
#include "mesh.h"
#include "camera.h"
#include "renderBufferBuilder.h"
#include "GizmoRenderer.h"

#undef max

inline const char* HEADER = "#version 450 core\n #define MAX_OBJECT_COUNT 64\n";

inline std::string ConstructShaderHeader(uint maxObjectCount) {
	std::string str;
	str.reserve(128);
	str += "#version 450 core\n #define MAX_OBJECT_COUNT ";
	str += std::to_string(maxObjectCount);
	str += '\n';
	return str;
}

const std::array<Vec3f, 8> TRI_CUBE_VERTICES = {
	Vec3f(0.5f, 0.5f, -0.5f),
	Vec3f(0.5f, -0.5f, -0.5f),
	Vec3f(0.5f, 0.5f, 0.5f),
	Vec3f(0.5f, -0.5f, 0.5f),
	Vec3f(-0.5f, 0.5f, -0.5f),
	Vec3f(-0.5f, -0.5f, -0.5f),
	Vec3f(-0.5f, 0.5f, 0.5f),
	Vec3f(-0.5f, -0.5f, 0.5f)
};

const std::array<Vec3u, 12> TRI_CUBE_ELEMENTS = {
	Vec3u(0, 4, 6),
	Vec3u(0, 6, 2),
	Vec3u(3, 2, 6),
	Vec3u(3, 6, 7),
	Vec3u(7, 6, 4),
	Vec3u(7, 4, 5),
	Vec3u(5, 1, 3),
	Vec3u(5, 3, 7),
	Vec3u(1, 0, 2),
	Vec3u(1, 2, 3),
	Vec3u(5, 4, 0),
	Vec3u(5, 0, 1)
};

constexpr float HALF_SQR_2 = 0.7071068f;
const std::vector<Vec3f> TRI_CYLINDER_VERTICES = {
	// Cylinder Base
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
	// Cylinder Top
	Vec3f(1.0f, 0.0f, 0.0f), Vec3f(HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(0.0f, 0.0f, -1.0f),
	Vec3f(-HALF_SQR_2, 0.0f, -HALF_SQR_2), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(-HALF_SQR_2, 0.0f, HALF_SQR_2),
	Vec3f(0.0f, 0.0f, 1.0f), Vec3f(HALF_SQR_2, 0.0f, HALF_SQR_2),
};

const std::vector<Vec3u> TRI_CYLINDER_ELEMENTS = {
	// Clinder Base
	Vec3u(0, 1, 2), Vec3u(0, 2, 3), Vec3u(0, 3, 4), Vec3u(0, 4, 5), Vec3u(0, 5, 6), Vec3u(0, 6, 7),
	// Cyclinder Top
	Vec3u(8, 10, 9), Vec3u(8, 11, 10), Vec3u(8, 12, 11), Vec3u(8, 13, 12), Vec3u(8, 14, 13), Vec3u(8, 15, 14),
	// Cylinder Sides
	Vec3u(0, 8, 1), Vec3u(1, 8, 9),
	Vec3u(1, 9, 2), Vec3u(2, 9, 10),
	Vec3u(2, 10, 3), Vec3u(3, 10, 11),
	Vec3u(3, 11, 4), Vec3u(4, 11, 12),
	Vec3u(4, 12, 5), Vec3u(5, 12, 13),
	Vec3u(5, 13, 6), Vec3u(6, 13, 14),
	Vec3u(6, 14, 7), Vec3u(7, 14, 15),
	Vec3u(7, 15, 0), Vec3u(0, 15, 8)
};

// Currently only renders one icon but can modify this later if we need to render more at a time
class IconRenderer {
	GLuint tex;
	GLuint vao;
	GLSLProgram prog;
	enum Uniform : int { TEX = 0, SCALE = 1, POS = 2, ASPECT_RATIO = 3 };

public:
	bool Initialize(int texBindPt, const uint8_t* icon, uint w, uint h, GLenum format = GL_RGBA) {
		bool success = prog.BuildFiles("assets/shaders/Icon.vert", "assets/shaders/Icon.frag");

		if (!success) {
			std::cout << "Failed to compile icon shader program!" << std::endl;
			return false;
		}

		glGenVertexArrays(1, &vao);
		glGenTextures(1, &tex);

		glActiveTexture(GLenum(GL_TEXTURE0) + GLenum(texBindPt));
		glBindTexture(GL_TEXTURE_2D, tex);

		if (icon) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, icon);
		}
		else {
			uint8_t red[4] = { 255, 0, 0, 255 };
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, red);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		prog.Bind();
		prog.RegisterUniform(TEX, "tex");
		prog.RegisterUniform(SCALE, "scale");
		prog.RegisterUniform(POS, "pos");
		prog.RegisterUniform(ASPECT_RATIO, "aspectR");

		prog.SetUniform(TEX, texBindPt);

		glActiveTexture(GL_TEXTURE0);

		return true;
	}

	void Draw(const Vec3f& pos, float scale, float aspectR) {
		glBindVertexArray(vao);
		prog.Bind();
		prog.SetUniform(POS, pos.x, pos.y, pos.z);
		prog.SetUniform(SCALE, scale * 0.5f);
		prog.SetUniform(ASPECT_RATIO, aspectR);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

};

// Owns the shared image shaders, quad geometry, image textures, and image pick pass.
class ImageRenderer {
	GLuint vao;
	int textureUnit = 0;
	GLSLProgram prog;
	GLSLProgram pickProg;

	// All reference images textures.
	struct TextureData {
		GLuint handle = 0;
		float aspect = 1.0f;
	};
	std::vector<TextureData> textures;

	// Buffers for the pick pass.
	GLuint pickFbo = 0;
	GLuint pickTex = 0;
	GLuint pickDepth = 0;
	GLuint pickRestoreFbo = 0;
	int pickWidth = 0;
	int pickHeight = 0;

	// Uniforms for visual and pick pass respectively.
	enum Uniform : int {
		TEX = 0, SCALE = 1, POS = 2, ROTATION = 3, IMAGE_ASPECT = 4,
		MVP = 5, OPACITY = 6, SELECTION_COLOR = 7, SELECTED = 8
	};
	enum PickUniform : int { PICK_SCALE = 0, PICK_POS = 1, PICK_ROTATION = 2, PICK_IMAGE_ASPECT = 3, PICK_MVP = 4, PICK_ID = 5 };

	/// <summary>
	/// Create or resize buffer for reference image picking.
	/// </summary>
	/// <param name="width"> Width of pick buffer. </param>
	/// <param name="height"> Height of pick buffer. </param>
	void ResizePickBuffer(int width, int height) {
		// Check for valid sizes, and if change occurred.
		if (width <= 0 || height <= 0) return;
		if (pickFbo != 0 && pickWidth == width && pickHeight == height) return;

		GLint previousActiveTexture = GL_TEXTURE0;
		GLint previousTexture2D = 0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);
		const GLuint oldPickTex = pickTex;

		// Delete old buffers.
		glDeleteFramebuffers(1, &pickFbo);
		glDeleteTextures(1, &pickTex);
		glDeleteRenderbuffers(1, &pickDepth);
		if (static_cast<GLuint>(previousTexture2D) == oldPickTex) {
			previousTexture2D = 0;
		}

		// Create new buffers.
		pickWidth = width;
		pickHeight = height;

		glGenFramebuffers(1, &pickFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, pickFbo);

		glGenTextures(1, &pickTex);
		glBindTexture(GL_TEXTURE_2D, pickTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0,
			GL_RGB_INTEGER, GL_UNSIGNED_INT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, pickTex, 0);

		glGenRenderbuffers(1, &pickDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, pickDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER, pickDepth);

		const GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cout << "Reference image pick framebuffer is invalid!" << std::endl;
		}

		glActiveTexture(static_cast<GLenum>(previousActiveTexture));
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture2D));
	}

public:
	/// <summary>
	/// Initialize image renderer and use texBindPt for the texture unit.
	/// </summary>
	/// <param name="texBindPt"></param>
	/// <returns></returns>
	bool Initialize(int texBindPt) {
		textureUnit = texBindPt;

		// Compile and error check shaders.
		bool success = prog.BuildFiles("assets/shaders/Image.vert", "assets/shaders/Image.frag");

		if (!success) {
			std::cout << "Failed to compile image shader program!" << std::endl;
			return false;
		}

		success = pickProg.BuildFiles("assets/shaders/Image.vert", "assets/shaders/ImagePick.frag");
		if (!success) {
			std::cout << "Failed to compile image picking shader program!" << std::endl;
			return false;
		}

		glGenVertexArrays(1, &vao);

		// Set visual uniforms.
		prog.Bind();
		prog.RegisterUniform(TEX, "tex");
		prog.RegisterUniform(SCALE, "scale");
		prog.RegisterUniform(POS, "pos");
		prog.RegisterUniform(ROTATION, "rotation");
		prog.RegisterUniform(IMAGE_ASPECT, "imageAspect");
		prog.RegisterUniform(MVP, "mvp");
		prog.RegisterUniform(OPACITY, "opacity");
		prog.RegisterUniform(SELECTION_COLOR, "selectionColor");
		prog.RegisterUniform(SELECTED, "selected");

		prog.SetUniform(TEX, texBindPt);

		// Set pick uniforms.
		pickProg.Bind();
		pickProg.RegisterUniform(PICK_SCALE, "scale");
		pickProg.RegisterUniform(PICK_POS, "pos");
		pickProg.RegisterUniform(PICK_ROTATION, "rotation");
		pickProg.RegisterUniform(PICK_IMAGE_ASPECT, "imageAspect");
		pickProg.RegisterUniform(PICK_MVP, "mvp");
		pickProg.RegisterUniform(PICK_ID, "imageId");

		glActiveTexture(GL_TEXTURE0);

		return true;
	}

	/// <summary>
	/// Add a reference image to the texture vector.
	/// </summary>
	/// <param name="image"> Pointer to image data. </param>
	/// <param name="w"> Width of image. </param>
	/// <param name="h"> Height of image. </param>
	/// <param name="format"></param>
	/// <returns> Index in texture vector. </returns>
	uint AddTexture(const uint8_t* image, uint w, uint h, GLenum format = GL_RGBA) {
		// Create entry and set fields.
		TextureData texture;
		texture.aspect = h > 0 ? float(w) / float(h) : 1.0f;

		// Create OpenGL texture.
		glGenTextures(1, &texture.handle);
		glActiveTexture(GLenum(GL_TEXTURE0) + GLenum(textureUnit));

		// Bind texture.
		glBindTexture(GL_TEXTURE_2D, texture.handle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Unbind texture.
		glActiveTexture(GL_TEXTURE0);

		// Add texture to vector, return index.
		textures.push_back(texture);
		return static_cast<uint>(textures.size() - 1);
	}

	/// <summary>
	/// Draw the visible pass.
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="rotation"></param>
	/// <param name="scale"></param>
	/// <param name="opacity"></param>
	/// <param name="selected"></param>
	/// <param name="selectionColor"></param>
	/// <param name="textureIndex"></param>
	void DrawVisible(const Vec3f& pos, const Vec3f& rotation, const Vec3f& scale, float opacity,
		bool selected, const Vec3f& selectionColor, uint textureIndex) {
		// Check for invalid texture.
		if (textureIndex >= textures.size()) return;

		// Bind texture at textureIndex.
		glActiveTexture(GLenum(GL_TEXTURE0) + GLenum(textureUnit));
		glBindTexture(GL_TEXTURE_2D, textures[textureIndex].handle);
		glBindVertexArray(vao);

		// Set uniforms.
		prog.Bind();
		prog.SetUniform(POS, pos.x, pos.y, pos.z);
		prog.SetUniform(SCALE, scale.x, scale.y, scale.z);
		prog.SetUniform(ROTATION, rotation.x, rotation.y, rotation.z);
		prog.SetUniform(IMAGE_ASPECT, textures[textureIndex].aspect);
		prog.SetUniform(OPACITY, opacity);
		prog.SetUniform3(SELECTION_COLOR, &selectionColor.x);
		prog.SetUniform(SELECTED, selected ? 1 : 0);

		// Draw quad.
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Unbind.
		glBindVertexArray(0);
		glActiveTexture(GL_TEXTURE0);
	}

	/// <summary>
	/// Setup pick pass, multiple draws for the pick pass can be called so only do setup once.
	/// </summary>
	/// <param name="depthSourceFbo"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	void BeginPickPass(GLuint depthSourceFbo, int width, int height) {
		// Check for valid size.
		if (width <= 0 || height <= 0) return;

		// Update pick buffer size.
		ResizePickBuffer(width, height);
		if (pickFbo == 0) return;
		
		// Check depth buffer to see if reference image is occluded and hence not pickable.
		pickRestoreFbo = depthSourceFbo;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, depthSourceFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pickFbo);
		glBlitFramebuffer(
			0, 0, pickWidth, pickHeight,
			0, 0, pickWidth, pickHeight,
			GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);

		glBindFramebuffer(GL_FRAMEBUFFER, pickFbo);
		glViewport(0, 0, pickWidth, pickHeight);
		const GLuint clearIds[4] = { 0, 0, 0, 0 };
		glClearBufferuiv(GL_COLOR, 0, clearIds);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	/// <summary>
	/// Draw pick for texture at textureIndex.
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="rotation"></param>
	/// <param name="scale"></param>
	/// <param name="imageId"> Image ID from scene. </param>
	/// <param name="textureIndex"> Index for texture in the texture vector. </param>
	void DrawPick(const Vec3f& pos, const Vec3f& rotation, const Vec3f& scale, uint imageId, uint textureIndex) {
		// Check valid texture and buffer.
		if (pickFbo == 0 || textureIndex >= textures.size()) return;

		glBindVertexArray(vao);

		// Set pick uniforms.
		pickProg.Bind();
		pickProg.SetUniform(PICK_POS, pos.x, pos.y, pos.z);
		pickProg.SetUniform(PICK_SCALE, scale.x, scale.y, scale.z);
		pickProg.SetUniform(PICK_ROTATION, rotation.x, rotation.y, rotation.z);
		pickProg.SetUniform(PICK_IMAGE_ASPECT, textures[textureIndex].aspect);
		pickProg.SetUniform(PICK_ID, imageId);

		// Draw pick quad.
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Unbind.
		glBindVertexArray(0);
	}

	/// <summary>
	/// Conclude pick pass.
	/// </summary>
	/// <param name="defaultFramebuffer"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	void EndPickPass(GLuint defaultFramebuffer, int width, int height) {
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
		glViewport(0, 0, width, height);
	}

	/// <summary>
	/// Draw visible texture.
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="rotation"></param>
	/// <param name="scale"></param>
	/// <param name="opacity"></param>
	/// <param name="selected"></param>
	/// <param name="selectionColor"></param>
	/// <param name="textureIndex"></param>
	/// <param name="defaultFramebuffer"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	void Draw(const Vec3f& pos, const Vec3f& rotation, const Vec3f& scale, float opacity, bool selected,
		const Vec3f& selectionColor, uint textureIndex,
		GLuint defaultFramebuffer, int width, int height) {
		// Check valid size and texture.
		if (width <= 0 || height <= 0 || textureIndex >= textures.size()) return;

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebuffer);
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		DrawVisible(pos, rotation, scale, opacity, selected, selectionColor, textureIndex);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
		glViewport(0, 0, width, height);
	}

	/// <summary>
	/// Get the image at x,y.
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <returns> Image ID at x,y. </returns>
	uint ImageIdAt(uint x, uint y) const {
		if (pickFbo == 0 || x >= static_cast<uint>(pickWidth) || y >= static_cast<uint>(pickHeight)) {
			return MAX_UINT;
		}

		Vec3u ids(MAX_UINT, MAX_UINT, MAX_UINT);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pickFbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &ids.x);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, pickRestoreFbo);
		return ids.x == REFERENCE_IMAGE_IDENTIFIER ? ids.y : MAX_UINT;
	}

	/// <summary>
	/// Upload model view projection matrix from camera.
	/// </summary>
	/// <param name="data"> MVP Matrix. </param>
	void SetTransform(float data[16]) {
		prog.Bind();
		prog.SetUniformMatrix4(MVP, data);
		pickProg.Bind();
		pickProg.SetUniformMatrix4(PICK_MVP, data);
	}

};

class VertexRenderer {
	GLuint vao;
	
	enum Uniform : int { SCALE = 0, VIEW_PROJ = 1 };
	enum Attribute : GLuint { VERT = 0, POS = 1, ID = 2 };

	struct Buffers {
		GLuint vbo;
		GLuint ebo;
		GLuint pos;
		GLuint id;
	} bufs;

	float scale = 0.1f;

	uint count;

	GLSLProgram prog;

public:
	bool Initialize(const char* header) {
		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate buffer handles
		glGenBuffers(4, &bufs.vbo);

		// Generate vertex buffer object for unit cube vertices
		glBindBuffer(GL_ARRAY_BUFFER, bufs.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * TRI_CUBE_VERTICES.size() * 3, TRI_CUBE_VERTICES.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(VERT);
		glVertexAttribPointer(VERT, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Generate element buffer object for unit cube triangles
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * TRI_CUBE_ELEMENTS.size() * 3, TRI_CUBE_ELEMENTS.data(), GL_STATIC_DRAW);

		// Generate instanced position buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.pos);
		glEnableVertexAttribArray(POS);
		glVertexAttribPointer(POS, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
		glVertexAttribDivisor(POS, 1);

		// Generate instanced id buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glEnableVertexAttribArray(ID);
		glVertexAttribIPointer(ID, 2, GL_UNSIGNED_INT, 0, (GLvoid*)0);
		glVertexAttribDivisor(ID, 1);

		// Unbind VAO & Buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		bool success = RecompileShaders(header);
		if (!success) { return false; }

		return true;
	}

	void Draw() {
		if (count == 0) { return; }

		glBindVertexArray(vao);

		prog.Bind();
		glDrawElementsInstanced(GL_TRIANGLES, TRI_CUBE_ELEMENTS.size() * 3, GL_UNSIGNED_INT, 0, count);

		glBindVertexArray(0);
	}

	bool RecompileShaders(const char* header) {
		bool success = prog.BuildFiles("assets/shaders/VertexDraw.vert", "assets/shaders/VertexDraw.frag",
			nullptr, nullptr, nullptr, header, &std::cout);

		if (!success) { std::cout << "Failed to compile vertex shader program!" << std::endl; return false; }

		prog.Bind();
		prog.RegisterUniform(VIEW_PROJ, "viewProj");
		prog.RegisterUniform(SCALE, "scale");

		prog.SetUniform(SCALE, scale);

		return success;
	}

	void SetScale(float scale) {
		this->scale = scale;
		prog.Bind();
		prog.SetUniform(SCALE, this->scale);
	}

	void SetViewProjection(float data[16]) {
		prog.Bind();
		prog.SetUniformMatrix4(VIEW_PROJ, data);
	}

	void SetVertices(const std::vector<Vec2u>& ids, const std::vector<Vec3f>& pos, GLenum hint = GL_STATIC_DRAW) {
		if (ids.size() != pos.size()) { return; }

		count = uint(ids.size());

		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * ids.size() * 2, ids.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, bufs.pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pos.size() * 3, pos.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

class EdgeRenderer {
	GLuint vao;

	enum Uniform : int { RADIUS = 1, VIEW_PROJ = 2 };
	enum Attribute : GLuint { POS = 0, P0 = 1, P1 = 2, ID = 3 };

	struct Buffers {
		GLuint vbo;
		GLuint ebo;
		GLuint edge;
		GLuint id;
	} bufs;

	float radius = 0.01f;

	uint count;

	GLSLProgram prog;

public:
	bool Initialize(const char* header) { 
		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate buffer handles
		glGenBuffers(4, &bufs.vbo);

		// Generate vertex buffer object for unit cube vertices
		glBindBuffer(GL_ARRAY_BUFFER, bufs.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * TRI_CYLINDER_VERTICES.size() * 3, TRI_CYLINDER_VERTICES.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(POS);
		glVertexAttribPointer(POS, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

		// Generate element buffer object for unit cube triangles
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * TRI_CYLINDER_ELEMENTS.size() * 3, TRI_CYLINDER_ELEMENTS.data(), GL_STATIC_DRAW);

		// Generate instanced position buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.edge);

		glEnableVertexAttribArray(P0);
		glVertexAttribPointer(P0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f) * 2, (GLvoid*)0);
		glVertexAttribDivisor(P0, 1);

		glEnableVertexAttribArray(P1);
		glVertexAttribPointer(P1, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f) * 2, (GLvoid*)sizeof(Vec3f));
		glVertexAttribDivisor(P1, 1);

		// Generate instanced id buffer
		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glEnableVertexAttribArray(ID);
		glVertexAttribIPointer(ID, 2, GL_UNSIGNED_INT, 0, (GLvoid*)0);
		glVertexAttribDivisor(ID, 1);

		// Unbind VAO & Buffers
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		bool success = RecompileShaders(header);
		if (!success) { return false; }

		return true;
	}

	void Draw() {
		if (count == 0) { return; }

		glBindVertexArray(vao);

		prog.Bind();
		glDrawElementsInstanced(GL_TRIANGLES, TRI_CYLINDER_ELEMENTS.size() * 3, GL_UNSIGNED_INT, 0, count);

		glBindVertexArray(0);
	}

	bool RecompileShaders(const char* header) {
		bool success = prog.BuildFiles("assets/shaders/EdgeDraw.vert", "assets/shaders/EdgeDraw.frag",
			nullptr, nullptr, nullptr, header, &std::cout);

		std::cout << success << std::endl;

		if (!success) { std::cout << "Failed to compile edge shader program!" << std::endl; return false; }

		prog.Bind();
		prog.RegisterUniform(RADIUS, "radius");
		prog.RegisterUniform(VIEW_PROJ, "viewProj");

		prog.SetUniform(RADIUS, radius);

		return success;
	}

	void SetRadius(float radius) {
		this->radius = radius;
		prog.Bind();
		prog.SetUniform(RADIUS, radius);
	}

	void SetViewProjection(float data[16]) {
		prog.Bind();
		prog.SetUniformMatrix4(VIEW_PROJ, data);
	}

	void SetEdges(const std::vector<Vec2u>& ids, const std::vector<Vec3f>& edges, GLenum hint = GL_STATIC_DRAW) {
		if (ids.size() != edges.size() / 2) { return; }

		count = uint(ids.size());

		glBindBuffer(GL_ARRAY_BUFFER, bufs.id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uint) * ids.size() * 2, ids.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, bufs.edge);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * edges.size() * 3, edges.data(), hint);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};

class GridRenderer {
	GLSLProgram prog;
	GLuint vao;
public:
	bool Initialize() {
		glGenVertexArrays(1, &vao);

		bool success = prog.BuildFiles("assets/shaders/Grid.vert", "assets/shaders/Grid.frag");
		if (!success) {
			std::cout << "Failed to compile grid shader program!" << std::endl;
			return false;
		}

		prog.Bind();
		prog.RegisterUniform(0, "mvp");

		return true;
	}

	void Draw() {
		glBindVertexArray(vao);
		prog.Bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void SetTransform(float data [16]) {
		prog.Bind();
		prog.SetUniformMatrix4(0, data);
	}
};

class ABC3DRenderer {

	struct Renderers {
		GridRenderer grid;
		VertexRenderer vert;
		EdgeRenderer edge;
		GizmoRenderer gizmo;
	} rndrs;

	enum TexBindings : int { POS = 0, NORM = 1, ID = 2 };

	enum Uniforms : uint { VIEW = 0 };

	struct Textures {
		GLuint geom;
		GLuint norm;
		GLuint id;
	} tex;

	struct VertexArrayObjects {
		GLuint mesh;
		GLuint frame;
	} vao;

	struct Buffers {
		GLuint meshVBO;
		GLuint matUBO;
		GLuint clrSSBO;
		void Initialize(){ 
			glGenBuffers(sizeof(Buffers) / sizeof(GLuint), &meshVBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, clrSSBO);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, matUBO);
		}
	} bufs;

	struct Programs {
		GLSLProgram mesh;
		GLSLProgram shade;
	} prog;

	struct FrameBuffer {
		int width = 0;
		int height = 0;
		GLuint fbo = 0;
		GLuint depthBuf = 0;
		GLuint gizmoPickFbo = 0;
		GLuint gizmoPickTex = 0;
		GLuint gizmoPickDepth = 0;
	} frame;

	struct CameraSettings {
		OrbitCamera orbit;
		FreeCamera free;
		float fov = 70.0f;
		CameraType type = ORBIT;
	} cam;

	struct VisibleSettings {
		bool vertices = false;
		bool grid = true;
		bool edges = true;
		bool surfaces = true;
	} show;

	struct UpdateQueue {
		bool renderBufs = false;
		bool camera = false;
		bool uniformBuf = false;
		bool selection = false;
		bool hover = false;
		bool rerenderGeom = false;

		void ClearAll() {
			renderBufs = false;
			camera = false;
			uniformBuf = false;
			selection = false;
			hover = false;
			rerenderGeom = false;
		}
	} update;

	struct HoverItem {
		uint mesh = MAX_UINT;
		uint element = MAX_UINT;
	} hover;

	struct TransformCache {
		Matrix4f proj;
		Matrix4f view;
	} transform;

	struct Offsets {
		uint faces = 0;
		uint edges = 0;
		uint verts = 0;
		std::vector<uint> meshFaces;
		std::vector<uint> meshEdges;
		std::vector<uint> meshVerts;
		void Clear() { meshFaces.clear(); meshEdges.clear(); meshVerts.clear(); }
	} offset;

	struct Counts {
		uint faces;
		uint edges;
		uint verts;
	} count;

	struct BaseColors {
		Vec3f background = Vec3f(0.0f);
		Vec3f faces = Vec3f(1.0f, 0.0f, 0.0f);
		Vec3f edges = Vec3f(1.0f, 1.0f, 0.0f);
		Vec3f verts = Vec3f(1.0f, 0.0f, 1.0f);
		enum ColorUniforms : uint {
			ALL = 0, BACKGROUND = 1, FACES = 2, 
			EDGES = 3, VERTICES = 4
		};
	} colors;

	struct BlendColors {
		Vec3f select = Vec3f(0.0f, 1.0f, 0.0f);
		Vec3f hover = Vec3f(1.0f, 1.0f, 1.0f);
		Vec3f both = Vec3f(0.25f, 0.75f, 0.25f);
		enum BlendUniforms : uint {
			ALL = 0, SELECT = 1, HOVER = 2, BOTH = 3
		};
	} blends;

	bool initialized = false;
	bool customViewProjection = false;
	uint meshVertDrawCount;
	uint maxObjectCount = 64; //hard coded for now

	RenderBufferBuilder bufBuilder;

	std::vector<CompactTransform140> transformBuf;
	std::vector<uint> clrs;
	std::vector<std::vector<uint>> edgeIdMap;   // per mesh: half-edge id -> drawn (compact) edge id, or MAX_UINT
	std::vector<std::vector<uint>> edgeIdUnmap; // per mesh: drawn (compact) edge id -> half-edge id

	const Scene* scene;
	const SelectionManager* selecQuery;

public:
// ---------------------------------------------------------------------------------------------
// Core ----------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	bool Initialize(GLuint defaultFrameBuffer);
	void Resize(int width, int height, GLuint defaultFrameBuffer);
	void Draw(GLuint defaultFramebuffer);
	void DrawGizmos(GLuint defaultFramebuffer);
	void SetViewProjection(const Matrix4f& view, const Matrix4f& projection, float gizmoFov);
	void RestoreCameraViewProjection();

private:
// ---------------------------------------------------------------------------------------------
// Buffer Updates ------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void UpdateRenderBuffers();
	void InitFramebuffer(GLuint defaultFrameBuffer);
	void UpdateUniformBuffer();
	void ResetColorBuffer();
	void UpdateSelection();
	void SetFaceBlendColor(uint meshId, uint faceId, uint8_t blendClr);
	void SetObjectBlendColor(uint meshId, uint8_t blendClr);
	void SetEdgeBlendColor(uint meshId, uint edgeId, uint8_t blendClr);
	void UpdateHover();

	void UploadColorBuffer() {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufs.clrSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, clrs.size() * sizeof(uint), clrs.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

// ---------------------------------------------------------------------------------------------
// Uniform Updates ----------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void UpdateProjectionUniforms();
	void UpdateViewUniforms();

public:
// ---------------------------------------------------------------------------------------------
// General API ---------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

	void UpdateTransforms() { update.uniformBuf = true; }
	void UpdateGeometry() { update.renderBufs = true; }

	void UpdateSelection(const SelectionManager& selectManager) { 
		update.selection = true; 
		selecQuery = &selectManager;
	}

	void UpdateHover(uint mesh, uint element) {
		hover.mesh = mesh;
		hover.element = element;
		update.hover = true;
	}

	void ClearHover() { hover.mesh = MAX_UINT; hover.element = MAX_UINT; update.hover = true; }

	void UpdateGizmos(const Vec3f& base, uint hoveredAxis, GizmoRenderer::Type type, const Matrix3f& rotation) {
		rndrs.gizmo.UpdateGizmos(base, hoveredAxis, type, rotation);
	}

	void ClearGizmos() { rndrs.gizmo.Clear(); }

	Vec3u GizmoSelectionIds(uint x, uint y) {
		if (x >= frame.width || y >= frame.height || frame.gizmoPickFbo == 0) {
			return Vec3u(MAX_UINT);
		}
		Vec3u ids;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame.gizmoPickFbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &ids.x);
		return ids;
	}

	void SetScene(const Scene& scene) { this->scene = &scene; }

	void ToggleGridVisibility() { show.grid = !show.grid; }
	void ToggleVertexVisibility() { show.vertices = !show.vertices; update.rerenderGeom = true; }
	void ToggleEdgeVisibility() { show.edges = !show.edges; update.rerenderGeom = true; }
	void ToggleSurfaceVisibility() { show.surfaces = !show.surfaces; update.rerenderGeom = true; }

	void ShowGrid(bool visible) { show.grid = visible; }
	void ShowVertices(bool visible) { show.vertices = visible; update.rerenderGeom = true; }
	void ShowEdges(bool visible) { show.edges = visible; update.rerenderGeom = true; }
	void ShowSurfaces(bool visible) { show.surfaces = visible; update.rerenderGeom = true; }

	bool ShowGrid() { return show.grid; }
	bool ShowVertices() { return show.vertices; }
	bool ShowEdges() { return show.edges; }
	bool ShowSurfaces() { return show.surfaces; }

	bool Initialized() { return initialized; }
	const Vec3f& SelectionColor() const { return blends.select; }

	void mouseDrag(const Vec2f& drag) {
		if (cam.type == ORBIT) { cam.orbit.mouseDrag(drag); }
		else { cam.free.mouseDrag(drag); }
		update.camera = true;
	}

	void mouseWheel(float dx, float dy) {
		if (cam.type == ORBIT) { cam.orbit.mouseWheel(dx, dy); }
		else { cam.free.mouseWheel(dx, dy); }
		update.camera = true;
	}

	void keyPress(uint8_t axis) {
		if (cam.type == ORBIT) { cam.orbit.keyPress(axis); }
		else { cam.free.keyPress(axis); }
		update.camera = true;
	}

	CameraType GetActiveCamType() const { return cam.type; }

	void setCameraPosition(const Vec3f& pos) {
		if (cam.type == ORBIT) { cam.orbit.setPosition(pos); }
		else { cam.free.setPosition(pos); }
		update.camera = true;
	}

	void setOrbitOrigin(const Vec3f& origin) { setCameraMode(CameraType::ORBIT); cam.orbit.setOrigin(origin); update.camera = true; }

	void setPrimaryCamParam(const Vec3f& param) {
		if (cam.type == ORBIT) {
			cam.orbit.setOrigin(param);
		}
		else {
			cam.free.setPosition(param);
		}

		update.camera = true;
	}

	Vec3f getPrimaryCamParam() {
		if (cam.type == ORBIT) {
			return cam.orbit.getOrigin();
		}
		else {
			return cam.free.position();
		}
	}

	const Camera* GetCamera() const { return cam.type == ORBIT ? (Camera*)&cam.orbit : (Camera*)&cam.free; }
	Vec3f GetCameraTarget() const { return cam.type == ORBIT ? cam.orbit.getOrigin() : cam.free.target(); }
	void GetActiveViewDirections(Vec3f& forward, Vec3f& up, Vec3f& right) const {
		const Matrix4f cameraTransform = transform.view.GetInverse();
		right = Vec3f(cameraTransform.Column(0));
		up = Vec3f(cameraTransform.Column(1));
		forward = -Vec3f(cameraTransform.Column(2));
		right.Normalize();
		up.Normalize();
		forward.Normalize();
	}

	void toggleCamType() {
		if (cam.type == ORBIT) {
			cam.type = FREE;
			cam.free.setPosition(cam.orbit.position());
			cam.free.setForward(cam.orbit.forward());
		}
		else {
			cam.type = ORBIT;
			cam.orbit.setOrigin(Vec3f(0.0f));
			cam.orbit.setPosition(cam.free.position());
		}

		update.camera = true;
	}

	void setCameraMode(CameraType mode) {
		if (mode == cam.type) { return; }
		toggleCamType();
	}

	void SetVertexScale(float scale) { rndrs.vert.SetScale(scale); update.rerenderGeom = true; }
	void SetEdgeRadius(float radius) { rndrs.edge.SetRadius(radius); update.rerenderGeom = true; }

	Vec3f ViewToWorldSpace(Vec3f viewPos) const {
		Vec3f worldPos;

		viewPos -= Vec3f(transform.view.Column(3));

		// multiply by transpose of 3x3 sub matrix (rotation matrix)

		worldPos.x = (	
			transform.view.cell[0] * viewPos[0] +
			transform.view.cell[1] * viewPos[1] +
			transform.view.cell[2] * viewPos[2] 
		);

		worldPos.y = (	
			transform.view.cell[4] * viewPos[0] +
			transform.view.cell[5] * viewPos[1] +
			transform.view.cell[6] * viewPos[2]
		);

		worldPos.z = (
			transform.view.cell[8] * viewPos[0] +
			transform.view.cell[9] * viewPos[1] +
			transform.view.cell[10] * viewPos[2]
		);

		return worldPos;
	}

	bool QueryFace(uint x, uint y, Vec2u* faceId, Vec3f* pos = nullptr) {
		if (x >= frame.width || y >= frame.height) { return false; }

		// read back ids
		Vec3u ids;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame.fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT2);
		glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &ids.x);

		if (ids.z != 1) { return false; }

		faceId->Set(ids.x, ids.y - offset.meshFaces[ids.x]);

		if (pos) {
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, &pos->x);
			*pos = ViewToWorldSpace(*pos);
		}

		return true;
	}

	Vec2u SelectionIds(Element element, uint x, uint y) {
		if (x >= frame.width || y >= frame.height) {
			return Vec2u(MAX_UINT);
		}

		// read back ids
		Vec3u ids;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame.fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT2);	
		glReadPixels(x, y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &ids.x);

		if (ids.z == 0) { return Vec2u(MAX_UINT); }

		switch (element) {
		case OBJECT:
			return Vec2u(ids.x, ids.z - 1);

		case FACE:
			if (ids.z != 1) { return Vec2u(ids.x, MAX_UINT); }
			return Vec2u(ids.x, ids.y - offset.meshFaces[ids.x]);

		case EDGE: {
			if (ids.z == 3) { return Vec2u(ids.x, MAX_UINT); }
			if (ids.z == 2) {
				// The edge pass writes compact edge numbers; turn them back into half-edge ids
				const uint compact = ids.y - offset.meshEdges[ids.x];
				if (ids.x < edgeIdUnmap.size() && compact < edgeIdUnmap[ids.x].size()) {
					return Vec2u(ids.x, edgeIdUnmap[ids.x][compact]);
				}
				return Vec2u(ids.x, MAX_UINT);
			}

			// read back pos tex and find closest edge in face
			Vec3f pos;
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, &pos.x);
			pos = ViewToWorldSpace(pos);

			const HalfEdgeMesh& mesh = *(scene->operator[](ids.x));
			pos = mesh.WorldPosToLocalPos(pos);

			return Vec2u(ids.x, mesh.NearestEdge(ids.y - offset.meshFaces[ids.x], pos));
		}
		case VERTEX: {
			if (ids.z == 2) { return Vec2u(ids.x, MAX_UINT); }
			if (ids.z == 3) { return Vec2u(ids.x, ids.y - offset.meshVerts[ids.x]); }

			// read pack pos tex and find closest vertex in face
			Vec3f pos;
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(x, y, 1, 1, GL_RGB, GL_FLOAT, &pos.x);
			pos = ViewToWorldSpace(pos);

			const HalfEdgeMesh& mesh = *(scene->operator[](ids.x));
			pos = mesh.WorldPosToLocalPos(pos);

			return Vec2u(ids.x, mesh.NearestVertex(ids.y - offset.meshFaces[ids.x], pos));
		}
		}
	}

	template <bool uploadUniform>
	void FaceColor(const Vec3f& clr) {
		colors.faces = clr; 
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BaseColors::FACES, &colors.faces.x);
		}
	}

	template <bool uploadUniform>
	void EdgeColor(const Vec3f& clr) {
		colors.edges = clr;
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BaseColors::EDGES, &colors.edges.x);
		}
	}

	template <bool uploadUniform>
	void VertexColor(const Vec3f& clr) {
		colors.verts = clr;
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BaseColors::VERTICES, &colors.verts.x);
		}
	}

	template <bool uploadUniform>
	void BackgroundColor(const Vec3f& clr) {
		colors.background = clr; 
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BaseColors::BACKGROUND, &colors.background.x);
		}
	}

	template <bool uploadUniform>
	void SelectColor(const Vec3f& clr) {
		blends.select = clr; 
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BlendColors::SELECT + 5, &blends.select.x);
		}
	}

	template <bool uploadUniform>
	void HoverColor(const Vec3f& clr) {
		blends.hover = clr;
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BlendColors::HOVER + 5, &blends.hover.x);
		}
	}

	template <bool uploadUniform>
	void SelectHoverColor(const Vec3f& clr) {
		blends.both = clr;
		if constexpr (uploadUniform) {
			prog.shade.Bind();
			prog.shade.SetUniform3(BlendColors::BOTH + 5, &blends.both.x);
		}
	}

	template <bool uploadUniform>
	void FOV(float fov) { 
		cam.fov = fov; 
		if constexpr (uploadUniform) {
			UpdateProjectionUniforms();
			update.rerenderGeom = true;
		}
	}

	float FOV() { return cam.fov; }

	bool RecompileShaders() {
		std::string header = ConstructShaderHeader(maxObjectCount);

		bool success = prog.mesh.BuildFiles("assets/shaders/GeomPass.vert", "assets/shaders/GeomPass.frag",
			nullptr, nullptr, nullptr, header.data(), &std::cout);

		if (!success) { std::cout << "Failed to compile mesh shader program!" << std::endl; }

		prog.mesh.RegisterUniform(VIEW, "view");

		success = success && rndrs.vert.RecompileShaders(header.data());
		success = success && rndrs.edge.RecompileShaders(header.data());

		UpdateProjectionUniforms();
		UpdateViewUniforms();

		return success;
	}

	void GetViewProjection(float data[16]) const {
		Matrix4f viewProjection = transform.proj * transform.view;
		viewProjection.Get(data);
	}

	Vec3f CanonicalVolumePos(const Vec3f& worldPos) {
		Vec4f projPos = transform.proj * (transform.view * worldPos);
		return projPos.GetNonHomogeneous();
	}

	Vec2f CanonicalVolumePos2(const Vec3f& worldPos) {
		Vec4f projPos = transform.proj * (transform.view * worldPos);
		return Vec2f(projPos.x / projPos.w, projPos.y / projPos.w);
	}

	/// <returns>World -> clip transform (projection * view). Built from the live camera, so it is current
	/// even before the next Draw() refreshes the cached view matrix.</returns>
	Matrix4f ViewProjection() const { return transform.proj * GetCamera()->transform(); }
};

#endif

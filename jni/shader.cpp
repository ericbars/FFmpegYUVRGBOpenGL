#include "player.h"

#define BYTES_PER_FLOAT 4
#define POSITION_COMPONENT_COUNT 2
#define TEXTURE_COORDINATES_COMPONENT_COUNT 2
#define STRIDE ((POSITION_COMPONENT_COUNT + TEXTURE_COORDINATES_COMPONENT_COUNT)*BYTES_PER_FLOAT)

/* type specifies the Shader type: GL_VERTEX_SHADER or GL_FRAGMENT_SHADER */
GLuint LoadShader(GLenum type, const char *shaderSrc) {
	GLuint shader;
	GLint compiled;

	// Create an empty shader object, which maintain the source code strings that define a shader
	shader = glCreateShader(type);

	if (shader == 0)
		return 0;

	// Replaces the source code in a shader object
	glShaderSource(shader, 1, &shaderSrc, NULL);

	// Compile the shader object
	glCompileShader(shader);

	// Check the shader object compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1) {
			GLchar* infoLog = (GLchar*) malloc(sizeof(GLchar) * infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			LOGV("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

GLuint LoadProgram(const char *vShaderStr, const char *fShaderStr) {
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

	// Create the program object
	programObject = glCreateProgram();
	if (programObject == 0)
		return 0;

	// Attaches a shader object to a program object
	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);
	// Bind vPosition to attribute 0
	glBindAttribLocation(programObject, 0, "vPosition");
	// Link the program object
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked) {
		GLint infoLen = 0;

		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1) {
			GLchar* infoLog = (GLchar*) malloc(sizeof(GLchar) * infoLen);

			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			LOGV("Error linking program:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(programObject);
		return GL_FALSE;
	}

	// Free no longer needed shader resources
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

int CreateProgram() {
	GLuint programObject;

	GLbyte vShaderStr[] = "attribute vec4 a_Position;  			\n"
			"attribute vec2 a_TextureCoordinates;   \n"
			"varying vec2 v_TextureCoordinates;     \n"
			"void main()                            \n"
			"{                                      \n"
			"    v_TextureCoordinates = a_TextureCoordinates;   \n"
			"    gl_Position = a_Position;    \n"
			"}                                      \n";

	GLbyte fShaderStr[] =
			"precision highp float; 							\n"
					"varying vec2 v_TextureCoordinates;              	\n"
					"uniform sampler2D tex_y;  							\n"
					"uniform sampler2D tex_u;  							\n"
					"uniform sampler2D tex_v; 							\n"
					"void main()										\n"
#if 1
			"{                                            									\n"
			"  vec4 c = vec4((texture2D(tex_y, v_TextureCoordinates).r - 16./255.) * 1.164);\n"
			"  vec4 U = vec4(texture2D(tex_u, v_TextureCoordinates).r - 128./255.);			\n"
			"  vec4 V = vec4(texture2D(tex_v, v_TextureCoordinates).r - 128./255.);			\n"
			"  c += V * vec4(1.596, -0.813, 0, 0);											\n"
			"  c += U * vec4(0, -0.392, 2.017, 0);											\n"
			"  c.a = 1.0;																	\n"
			"  gl_FragColor = c;															\n"
			"}                                            									\n";
#else
	"{													\n"
	"	    highp float y = texture2D(tex_y, v_TextureCoordinates).r;  			\n"
	"	    highp float u = texture2D(tex_u, v_TextureCoordinates).r - 0.5;  	\n"
	"	    highp float v = texture2D(tex_v, v_TextureCoordinates).r - 0.5;  	\n"
	"		highp float r = y + 1.402 * v;										\n"
	"		highp float g = y - 0.344 * u - 0.714 * v;							\n"
	"		highp float b = y + 1.772 * u;										\n"
	"		gl_FragColor = vec4(r, g, b, 1.0);									\n"
	"}																			\n";
#endif

	// Load the shaders and get a linked program object
	programObject = LoadProgram((const char*) vShaderStr,
			(const char*) fShaderStr);
	if (programObject == 0) {
		return GL_FALSE;
	}

	// Store the program object
	global_context.glProgram = programObject;

	// Get the attribute locations
	global_context.positionLoc = glGetAttribLocation(programObject,
			"v_position");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glGenTextures(3, global_context.mTextureID);
	for (int i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, global_context.mTextureID[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	return 0;
}

void Render(AVFrame *frame) {
	GLfloat vVertices[] = { 0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
			0.0f };
	// Clear the color buffer
	//glClear(GL_COLOR_BUFFER_BIT);

	// Use the program object
	glUseProgram(global_context.glProgram);

	//Get Uniform Variables Location
	GLint textureUniformY = glGetUniformLocation(global_context.glProgram,
			"tex_y");
	GLint textureUniformU = glGetUniformLocation(global_context.glProgram,
			"tex_u");
	GLint textureUniformV = glGetUniformLocation(global_context.glProgram,
			"tex_v");

	int w = global_context.vcodec_ctx->width;
	int h = global_context.vcodec_ctx->height;
	GLubyte* y = (GLubyte*) frame->data[0];
	GLubyte* u = (GLubyte*) frame->data[1];
	GLubyte* v = (GLubyte*) frame->data[2];
	GLint y_width = frame->linesize[0];
	GLint u_width = frame->linesize[1];
	GLint v_width = frame->linesize[2];

	// Set the viewport
	glViewport(0, 0, y_width, global_context.vcodec_ctx->height);

	//Y
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, global_context.mTextureID[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, y_width, h, 0, GL_LUMINANCE,
			GL_UNSIGNED_BYTE, y);
	glUniform1i(textureUniformY, 0);
	//U
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, global_context.mTextureID[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, u_width, h / 2, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, u);
	glUniform1i(textureUniformU, 1);
	//V
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, global_context.mTextureID[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, v_width, h / 2, 0,
			GL_LUMINANCE, GL_UNSIGNED_BYTE, v);
	glUniform1i(textureUniformV, 2);

	// Retrieve attribute locations for the shader program.
	GLint aPositionLocation = glGetAttribLocation(global_context.glProgram,
			"a_Position");
	GLint aTextureCoordinatesLocation = glGetAttribLocation(
			global_context.glProgram, "a_TextureCoordinates");

	// Order of coordinates: X, Y, S, T
	// Triangle Fan
	GLfloat VERTEX_DATA[] = { 0.0f, 0.0f, 0.5f, 0.5f, -1.0f, -1.0f, 0.0f, 1.0f,
			1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
			0.0f, -1.0f, -1.0f, 0.0f, 1.0f };

	glVertexAttribPointer(aPositionLocation, POSITION_COMPONENT_COUNT, GL_FLOAT,
			false, STRIDE, VERTEX_DATA);
	glEnableVertexAttribArray(aPositionLocation);

	glVertexAttribPointer(aTextureCoordinatesLocation, POSITION_COMPONENT_COUNT,
			GL_FLOAT, false, STRIDE, &VERTEX_DATA[POSITION_COMPONENT_COUNT]);
	glEnableVertexAttribArray(aTextureCoordinatesLocation);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	eglSwapBuffers(global_context.eglDisplay, global_context.eglSurface);
}

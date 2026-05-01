#include "opengl_context.hpp"


#include <imgui.h>
#include <implot.h>


#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>


#include <iostream>
#include <exception>



namespace framework {


#if _DEBUG
	//see example on: https://www.khronos.org/opengl/wiki/Debug_Output
	void ErrorMessageCallback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		//exclude log messages such as "buffer will be put into video memory"
		/*
		if (
			(
				GL_DEBUG_SEVERITY_HIGH == severity ||
				GL_DEBUG_SEVERITY_MEDIUM
				//GL_DEBUG_SEVERITY_HIGH
				== severity)
			) {
			*/

		const char* typeStr = "Unknown";
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behavior"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behavior"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
		}

		const char* severityStr = "Unknown";
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         severityStr = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          severityStr = "Low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "Notification"; break;
		}

		if (GL_DEBUG_SEVERITY_HIGH == severity) {

			fprintf(stderr, "GL CALLBACK: %s type = %s, severity = %s, message = %s\n",
				(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
				typeStr, severityStr, message);
		}


			// Optionally, trigger a breakpoint in debug mode
#ifdef _DEBUG
			
			if (severity == GL_DEBUG_SEVERITY_HIGH)
			{
				__debugbreak();  // Windows specific, use `assert(false);` for other platforms
			}
#endif

		//}
	}
#endif

	OpenGLGraphicsContext::OpenGLGraphicsContext(glm::ivec2 const& requested_window_resolution) {
		create_window_(requested_window_resolution);
		initialize_();
		register_gui();
	}

	void OpenGLGraphicsContext::swap_buffer_and_poll_events() const {
		auto *window_handle = get_window_handle();
		glfwSwapBuffers(window_handle);
		// update events
		glfwPollEvents();
	}

	void OpenGLGraphicsContext::register_gui() const {
		IMGUI_CHECKVERSION();
		auto *imgui_context = ImGui::CreateContext();

		ImGui::SetCurrentContext(imgui_context);
		auto *implot_context = ImPlot::CreateContext();
		ImPlot::SetCurrentContext(implot_context);
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		// Setup Dear ImGui style
		//ImGui::StyleColorsDark();
		ImGui::StyleColorsLight();
		ImGui::StyleColorsClassic();
		// Setup Platform/Renderer backends


		{
			auto *window_handle = get_window_handle();
			ImGui_ImplGlfw_InitForOpenGL(window_handle, true);
			ImGui_ImplOpenGL3_Init("#version 460");
			//exit(-1);
		}
	}

	void OpenGLGraphicsContext::start_gui_frame() const {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();





	}

	void OpenGLGraphicsContext::end_gui_frame() const {
		

		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

	void OpenGLGraphicsContext::create_window_(glm::ivec2 const& requested_window_resolution) {

		// initialize GLFW
		if (!glfwInit()) {
			std::cout << "Did not succeed in initializing GLFW in the application compilation unit" << '\n';
			std::cout << "Exiting app" << '\n';
			exit(-1);
		}

		#if _DEBUG
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
				glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
		#else
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
				glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
				glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
				//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		#endif


		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	

		auto *window_to_assign = glfwCreateWindow(requested_window_resolution[0], requested_window_resolution[1], "[OpenGL-Mode] Isosurface Renderer", nullptr, nullptr);
		
		if (window_to_assign) {
			assign_window_(window_to_assign);
		}


		if (!window_to_assign)
		{
			printf("Could not create GLFW Window (requested resolution: %d, %d). Exiting.\n", requested_window_resolution[0], requested_window_resolution[1]);
			glfwTerminate();
			exit(-1);
		}

		// set current context
		glfwMakeContextCurrent(window_to_assign);
		glfwSwapInterval(/*app_descriptor_.swap_interval*/ 0 );
	
	}

	void OpenGLGraphicsContext::initialize_() {
		// init the extension wrangler
		if (gl3wInit()) {
			std::cerr << "failed to initialize OpenGL via gl3w" << '\n';
			throw std::exception();
		}

		if (!gl3wIsSupported(4, 6)) {
			std::cerr << "failed to initialize OpenGL via gl3w" << '\n';
			throw std::exception();
		}

#if _DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(ErrorMessageCallback, 0);
#endif

		register_GL_extensions_();
		query_GL_parameters_();

		// Enable parallel shader compilation (ARB_parallel_shader_compile)
		auto glMaxShaderCompilerThreads = (PFNGLMAXSHADERCOMPILERTHREADSARBPROC)gl3wGetProcAddress("glMaxShaderCompilerThreadsARB");
		if (glMaxShaderCompilerThreads) {
			glMaxShaderCompilerThreads(0); // 0 = use maximum available threads
		}
	}

	bool OpenGLGraphicsContext::is_in_OpenGL_mode_() {
		return true;
	}

	void OpenGLGraphicsContext::register_GL_extensions_() {
		loaded_api_extensions.glDrawMeshTasksNV = (PFNGLDRAWMESHTASKSNVPROC)gl3wGetProcAddress("glDrawMeshTasksNV");
		loaded_api_extensions.glDrawMeshTasksIndirectNV = (PFNGLDRAWMESHTASKSINDIRECTNVPROC)gl3wGetProcAddress("glDrawMeshTasksIndirectNV");
		loaded_api_extensions.glMultiDrawMeshTasksIndirectNV = (PFNGLMULTIDRAWMESHTASKSINDIRECTNVPROC)gl3wGetProcAddress("glMultiDrawMeshTasksIndirectNV");
		loaded_api_extensions.glMultiDrawMeshTasksIndirectCountNV = (PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTNVPROC)gl3wGetProcAddress("glMultiDrawMeshTasksIndirectCountNV");


		loaded_api_extensions.glDrawMeshTasksEXT = (PFNGLDRAWMESHTASKSEXTPROC)gl3wGetProcAddress("glDrawMeshTasksEXT");
		loaded_api_extensions.glDrawMeshTasksIndirectEXT = (PFNGLDRAWMESHTASKSINDIRECTEXTPROC)gl3wGetProcAddress("glDrawMeshTasksIndirectEXT");
		loaded_api_extensions.glMultiDrawMeshTasksIndirectEXT = (PFNGLMULTIDRAWMESHTASKSINDIRECTEXTPROC)gl3wGetProcAddress("glMultiDrawMeshTasksIndirectEXT");
		loaded_api_extensions.glMultiDrawMeshTasksIndirectCountEXT = (PFNGLMULTIDRAWMESHTASKSINDIRECTCOUNTEXTPROC)gl3wGetProcAddress("glMultiDrawMeshTasksIndirectCountEXT");

		loaded_api_extensions.glNamedStringARB = (PFNGLNAMEDSTRINGARBPROC)gl3wGetProcAddress("glNamedStringARB");
		loaded_api_extensions.glDeleteNamedStringARB = (PFNGLDELETENAMEDSTRINGARBPROC)gl3wGetProcAddress("glDeleteNamedStringARB");
		loaded_api_extensions.glCompileShaderIncludeARB = (PFNGLCOMPILESHADERINCLUDEARBPROC)gl3wGetProcAddress("glCompileShaderIncludeARB");

		loaded_api_extensions.glGetTextureHandleARB = (PFNGLGETTEXTUREHANDLEARBPROC)gl3wGetProcAddress("glGetTextureHandleARB");
		loaded_api_extensions.glMakeTextureHandleResidentARB = (PFNGLMAKETEXTUREHANDLERESIDENTARBPROC)gl3wGetProcAddress("glMakeTextureHandleResidentARB");
		loaded_api_extensions.glMakeTextureHandleNonResidentARB = (PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC)gl3wGetProcAddress("glMakeTextureHandleNonResidentARB");
		loaded_api_extensions.glIsTextureHandleResidentARB = (PFNGLISTEXTUREHANDLERESIDENTARBPROC)gl3wGetProcAddress("glIsTextureHandleResidentARB");
	}

	void OpenGLGraphicsContext::query_GL_parameters_() {
		for (int32_t dim_idx = 0; dim_idx < 3; ++dim_idx) {
			glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, dim_idx, &(queried_api_parameters.MAX_COMPUTE_WORK_GROUP_COUNTS[dim_idx]));

			if (dim_idx < 2) {
				queried_api_parameters.MAX_COMPUTE_WORK_GROUP_COUNTS_PER_SLICE_AND_VOLUME[0] *= queried_api_parameters.MAX_COMPUTE_WORK_GROUP_COUNTS[dim_idx];
			}
			else {
				queried_api_parameters.MAX_COMPUTE_WORK_GROUP_COUNTS_PER_SLICE_AND_VOLUME[1] = queried_api_parameters.MAX_COMPUTE_WORK_GROUP_COUNTS_PER_SLICE_AND_VOLUME[0] * queried_api_parameters.MAX_COMPUTE_WORK_GROUP_COUNTS[dim_idx];
			}

		}


	}
}

@echo off
@echo ------------------------------------------
@echo Vulkan SDK path: %Vulkan_SDK%
@echo ------------------------------------------
@echo Compiling basic.glsl vertex stage
%Vulkan_SDK%/Bin/glslc.exe basic.glsl -o vertexshader.spv -D_VERTEX_SHADER=1
@echo ------------------------------------------
@echo Compiling basic.glsl fragment stage
%Vulkan_SDK%/Bin/glslc.exe basic.glsl -o fragmentshader.spv -D_FRAGMENT_SHADER=1
@echo ------------------------------------------
@echo Compiling model.glsl vertex stage
%Vulkan_SDK%/Bin/glslc.exe model.glsl -o model_vs.spv -D_VERTEX_SHADER=1
@echo ------------------------------------------
@echo Compiling model.glsl fragment stage
%Vulkan_SDK%/Bin/glslc.exe model.glsl -o model_fs.spv -D_FRAGMENT_SHADER=1








@REM glslc.exe options:
@REM   -c                Only run preprocess, compile, and assemble steps.
@REM   -Dmacro[=defn]    Add an implicit macro definition.
@REM   -E                Outputs only the results of the preprocessing step.
@REM                     Output defaults to standard output.
@REM   -fauto-bind-uniforms
@REM                     Automatically assign bindings to uniform variables that
@REM                     don't have an explicit 'binding' layout in the shader
@REM                     source.
@REM   -fauto-map-locations
@REM                     Automatically assign locations to uniform variables that
@REM                     don't have an explicit 'location' layout in the shader
@REM                     source.
@REM   -fauto-combined-image-sampler
@REM                     Removes sampler variables and converts existing textures
@REM                     to combined image-samplers.
@REM   -fentry-point=<name>
@REM                     Specify the entry point name for HLSL compilation, for
@REM                     all subsequent source files.  Default is "main".
@REM   -fhlsl-16bit-types
@REM                     Enable 16-bit type support for HLSL.
@REM   -fhlsl_functionality1, -fhlsl-functionality1
@REM                     Enable extension SPV_GOOGLE_hlsl_functionality1 for HLSL
@REM                     compilation.
@REM   -fhlsl-iomap      Use HLSL IO mappings for bindings.
@REM   -fhlsl-offsets    Use HLSL offset rules for packing members of blocks.
@REM                     Affects only GLSL.  HLSL rules are always used for HLSL.
@REM   -finvert-y        Invert position.Y output in vertex shader.
@REM   -flimit=<settings>
@REM                     Specify resource limits. Each limit is specified by a limit
@REM                     name followed by an integer value.  Tokens should be
@REM                     separated by whitespace.  If the same limit is specified
@REM                     several times, only the last setting takes effect.
@REM   -flimit-file <file>
@REM                     Set limits as specified in the given file.
@REM   -fnan-clamp       Generate code for max and min builtins so that, when given
@REM                     a NaN operand, the other operand is returned. Similarly,
@REM                     the clamp builtin will favour the non-NaN operands, as if
@REM                     clamp were implemented as a composition of max and min.
@REM   -fpreserve-bindings
@REM                     Preserve all binding declarations, even if those bindings
@REM                     are not used.
@REM   -fresource-set-binding [stage] <reg0> <set0> <binding0>
@REM                         [<reg1> <set1> <binding1>...]
@REM                     Explicitly sets the descriptor set and binding for
@REM                     HLSL resources, by register name.  Optionally restrict
@REM                     it to a single stage.
@REM   -fcbuffer-binding-base [stage] <value>
@REM                     Same as -fubo-binding-base.
@REM   -fimage-binding-base [stage] <value>
@REM                     Sets the lowest automatically assigned binding number for
@REM                     images.  Optionally only set it for a single shader stage.
@REM                     For HLSL, the resource register number is added to this
@REM                     base.
@REM   -fsampler-binding-base [stage] <value>
@REM                     Sets the lowest automatically assigned binding number for
@REM                     samplers  Optionally only set it for a single shader stage.
@REM                     For HLSL, the resource register number is added to this
@REM                     base.
@REM   -fssbo-binding-base [stage] <value>
@REM                     Sets the lowest automatically assigned binding number for
@REM                     shader storage buffer objects (SSBO).  Optionally only set
@REM                     it for a single shader stage.  Only affects GLSL.
@REM   -ftexture-binding-base [stage] <value>
@REM                     Sets the lowest automatically assigned binding number for
@REM                     textures.  Optionally only set it for a single shader stage.
@REM                     For HLSL, the resource register number is added to this
@REM                     base.
@REM   -fuav-binding-base [stage] <value>
@REM                     For automatically assigned bindings for unordered access
@REM                     views (UAV), the register number is added to this base to
@REM                     determine the binding number.  Optionally only set it for
@REM                     a single shader stage.  Only affects HLSL.
@REM   -fubo-binding-base [stage] <value>
@REM                     Sets the lowest automatically assigned binding number for
@REM                     uniform buffer objects (UBO).  Optionally only set it for
@REM                     a single shader stage.
@REM                     For HLSL, the resource register number is added to this
@REM                     base.
@REM   -fshader-stage=<stage>
@REM                     Treat subsequent input files as having stage <stage>.
@REM                     Valid stages are vertex, vert, fragment, frag, tesscontrol,
@REM                     tesc, tesseval, tese, geometry, geom, compute, and comp.
@REM   -g                Generate source-level debug information.
@REM   -h                Display available options.
@REM   --help            Display available options.
@REM   -I <value>        Add directory to include search path.
@REM   -mfmt=<format>    Output SPIR-V binary code using the selected format. This
@REM                     option may be specified only when the compilation output is
@REM                     in SPIR-V binary code form. Available options are:
@REM                       bin   - SPIR-V binary words.  This is the default.
@REM                       c     - Binary words as C initializer list of 32-bit ints
@REM                       num   - List of comma-separated 32-bit hex integers
@REM   -M                Generate make dependencies. Implies -E and -w.
@REM   -MM               An alias for -M.
@REM   -MD               Generate make dependencies and compile.
@REM   -MF <file>        Write dependency output to the given file.
@REM   -MT <target>      Specify the target of the rule emitted by dependency
@REM                     generation.
@REM   -O                Optimize the generated SPIR-V code for better performance.
@REM   -Os               Optimize the generated SPIR-V code for smaller size.
@REM   -O0               Disable optimization.
@REM   -o <file>         Write output to <file>.
@REM                     A file name of '-' represents standard output.
@REM   -std=<value>      Version and profile for GLSL input files. Possible values
@REM                     are concatenations of version and profile, e.g. 310es,
@REM                     450core, etc.  Ignored for HLSL files.
@REM   -S                Emit SPIR-V assembly instead of binary.
@REM   --show-limits     Display available limit names and their default values.
@REM   --target-env=<environment>
@REM                     Set the target client environment, and the semantics
@REM                     of warnings and errors.  An optional suffix can specify
@REM                     the client version.  Values are:
@REM                         vulkan1.0       # The default
@REM                         vulkan1.1
@REM                         vulkan1.2
@REM                         vulkan1.3
@REM                         vulkan          # Same as vulkan1.0
@REM                         opengl4.5
@REM                         opengl          # Same as opengl4.5
@REM   --target-spv=<spirv-version>
@REM                     Set the SPIR-V version to be used for the generated SPIR-V
@REM                     module.  The default is the highest version of SPIR-V
@REM                     required to be supported for the target environment.
@REM                     For example, default for vulkan1.0 is spv1.0, and
@REM                     the default for vulkan1.1 is spv1.3,
@REM                     the default for vulkan1.2 is spv1.5.
@REM                     the default for vulkan1.3 is spv1.6.
@REM                     Values are:
@REM                         spv1.0, spv1.1, spv1.2, spv1.3, spv1.4, spv1.5, spv1.6
@REM   --version         Display compiler version information.
@REM   -w                Suppresses all warning messages.
@REM   -Werror           Treat all warnings as errors.
@REM   -x <language>     Treat subsequent input files as having type <language>.
@REM                     Valid languages are: glsl, hlsl.
@REM                     For files ending in .hlsl the default is hlsl.
@REM                     Otherwise the default is glsl.
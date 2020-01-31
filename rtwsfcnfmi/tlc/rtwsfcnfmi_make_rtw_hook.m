function rtwsfcnfmi_make_rtw_hook(hookMethod, modelName, rtwRoot, templateMakefile, buildOpts, buildArgs, buildInfo)

switch hookMethod
    
    case 'after_make'
        
        % skip if build is disabled
        if strcmp(get_param(gcs, 'GenCodeOnly'), 'on')
            return
        end
        
        pathstr = which('rtwsfcnfmi.tlc');
        [tlc_dir, ~, ~] = fileparts(pathstr);
        [cmakelists_dir, ~, ~] = fileparts(tlc_dir);
        command = get_param(modelName, 'CMakeCommand');
        command = grtfmi_find_cmake(command);
        generator = get_param(modelName, 'CMakeGenerator');
        
        % MATLAB version for conditional compilation
        if verLessThan('matlab', '7.12')
            matlab_version = '';  % do nothing
        elseif verLessThan('matlab', '8.5')
            matlab_version = 'MATLAB_R2011a_';  % R2011a - R2014b
        elseif verLessThan('matlab', '9.3')
            matlab_version = 'MATLAB_R2015a_';  % R2015a - R2017a
        else
            matlab_version = 'MATLAB_R2017b_';  % R2017b - R2018b
        end
        
        custom_source = get_param(gcs, 'CustomSource');
        custom_source = which(custom_source);
        
        solver = buildOpts.solver;
        if ~strcmp(solver, {'ode1', 'ode2', 'ode3', 'ode4', 'ode5', 'ode8', 'ode14x'})
            solver = 'ode1';  % use ode1 for model exchange
        end
        
        % get model sources
        [custom_include, custom_source, custom_library] = ...
            rtwsfcnfmi_model_sources(modelName, pwd);
        
        custom_include = cmake_list(custom_include);
        custom_source  = cmake_list(custom_source);
        custom_library = cmake_list(custom_library);
        
        % write the CMakeCache.txt file
        fid = fopen('CMakeCache.txt', 'w');
        fprintf(fid, 'MODEL_NAME:STRING=%s\n', modelName);
        fprintf(fid, 'SOLVER:STRING=%s\n', solver);
        fprintf(fid, 'RTW_DIR:STRING=%s\n', strrep(pwd, '\', '/'));
        fprintf(fid, 'MATLAB_ROOT:STRING=%s\n', strrep(matlabroot, '\', '/'));
        fprintf(fid, 'MATLAB_VERSION:STRING=%s\n', matlab_version);
        fprintf(fid, 'CUSTOM_INCLUDE:STRING=%s\n', custom_include);
        fprintf(fid, 'CUSTOM_SOURCE:STRING=%s\n', custom_source);
        fprintf(fid, 'CUSTOM_LIBRARY:STRING=%s\n', custom_library);
        %fprintf(fid, 'COMPILER_OPTIMIZATION_LEVEL:STRING=%s\n', get_param(gcs, 'CMakeCompilerOptimizationLevel'));
        %fprintf(fid, 'COMPILER_OPTIMIZATION_FLAGS:STRING=%s\n', get_param(gcs, 'CMakeCompilerOptimizationFlags'));
        fclose(fid);
        
        disp('### Generating project')
        status = system(['"' command '" -G "' generator '" "' strrep(cmakelists_dir, '\', '/') '"']);
        assert(status == 0, 'Failed to run CMake generator');

        disp('### Building FMU')
        status = system(['"' command '" --build . --config Release']);
        assert(status == 0, 'Failed to build FMU');
        
        % copy the FMU to the working directory
        copyfile([modelName '.fmu'], '..');
end

end


function joined = cmake_list(array)

if isempty(array)
    joined = '';
    return
end

joined = array{1};

for i = 2:numel(array)
    joined = [joined ';' array{i}];  %#ok<ARGROW>
end

end

models = {
  {'f14',            '60', 'alpharad'}, ...
  {'sldemo_bounce',  '25', 'ContinuousStates.Position[1]'}, ...
  {'sldemo_clutch',  '10', 'w'}, ...
  {'sldemo_engine',  '10', 'crankspeedradsec'}, ...
  {'sldemo_fuelsys', '10', 'ContinuousStates.StateSpace_CSTATE'}, ...
};

figure

for i = 1:numel(models)
    model     = models{i}{1};
    stop_time = models{i}{2};
    outputs   = models{i}{3};
    rtwsfcnfmi_export_model(model, 'LoadBinaryMEX', 'off', 'CMakeGenerator', 'Visual Studio 14 2015 Win64');
    system(['fmpy --stop-time ' stop_time ' --output-variables ' outputs ' --output-file ' model '_out.csv simulate ' model '.fmu']);
    m = csvread([model '_out.csv'], 1);
    subplot(numel(models), 1, i);
    plot(m(:,1), m(:,2:end));
end

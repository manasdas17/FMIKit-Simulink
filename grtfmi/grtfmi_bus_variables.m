function variables = grtfmi_bus_variables(dataType)

variables = {};

% check if dataType starts with "Bus: "
if ~strncmp(dataType, 'Bus: ', numel('Bus: '))
  return
end

bus_name = dataType(numel('Bus: ')+1:end);

bus = evalin('base', bus_name);

for i = 1:numel(bus.Elements)
  
  element = bus.Elements(i);
  
  if strncmp(element.DataType, 'Bus: ', numel('Bus: '))
    subVars = grtfmi_bus_variables(element.DataType);
    for j = 1:numel(subVars)
      subVar = subVars{j};
      variables{end+1} = {[element.Name '.' subVar{1}], subVar{2}};
    end

  end
  
  switch element.DataType
    case 'double'
      dtypeID = 0;  % SS_DOUBLE
    case 'single'
      dtypeID = 1;  % SS_SINGLE
    case 'int8'
      dtypeID = 2;  % SS_INT8
    case 'uint8'
      dtypeID = 3;  % SS_UINT8
    case 'int16'
      dtypeID = 4;  % SS_INT16
    case 'uin16'
      dtypeID = 5;  % SS_UINT16
    case 'int32'
      dtypeID = 6;  % SS_INT32
    case 'uint32'
      dtypeID = 7;  % SS_UINT32
    case 'boolean'
      dtypeID = 8;  % SS_BOOLEAN
    otherwise
      continue
  end
  
  variables{end+1} = { element.Name, dtypeID };

end

end

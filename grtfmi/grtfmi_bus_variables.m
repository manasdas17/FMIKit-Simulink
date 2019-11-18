function variables = grtfmi_bus_variables(dataType)

% bus_name = get_param(sys, 'OutDataTypeStr');

bus_name = dataType(numel('Bus: ')+1:end);

bus = evalin('base', bus_name);

variables = {};

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
      dtypeID = 0;
    case 'single'
      dtypeID = 1;
    otherwise
      continue
  end
  
%     SS_DOUBLE  =  0,    /* real_T    */
%     SS_SINGLE  =  1,    /* real32_T  */
%     SS_INT8    =  2,    /* int8_T    */
%     SS_UINT8   =  3,    /* uint8_T   */
%     SS_INT16   =  4,    /* int16_T   */
%     SS_UINT16  =  5,    /* uint16_T  */
%     SS_INT32   =  6,    /* int32_T   */
%     SS_UINT32  =  7,    /* uint32_T  */
%     SS_BOOLEAN =  8     /* boolean_T */
  
  variables{end+1} = { element.Name, dtypeID };
%   variables{end+1} = { ...
%     bus.Elements(i).Name, ...
%     bus.Elements(i).DataType, ...
%   };
end

end

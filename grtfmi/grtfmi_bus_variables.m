function variables = grtfmi_bus_variables(sys)

bus_name = get_param(sys, 'OutDataTypeStr');

bus_name = bus_name(numel('Bus: ')+1:end);

bus = evalin('base', bus_name);

variables = {};

for i = 1:numel(bus.Elements)
  
  element = bus.Elements(i);
  
  switch element.DataType
    case 'double'
      dtypeID = 'SS_DOUBLE';
    case 'single'
      dtypeID = 'SS_SINGLE';
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

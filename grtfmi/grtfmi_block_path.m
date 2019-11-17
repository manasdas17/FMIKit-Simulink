function variable_name = grtfmi_block_path(p)

disp(p)

segments = {};

i = 1;
j = 1;

while j <= numel(p)
  if p(j) == '/'
    if p(j+1) == '/'
      j = j + 2;
      continue
    end
    segments{end+1} = p(i+1:j-1);
    i = j;
  elseif j == numel(p)
    segments{end+1} = p(i+1:j);
  end
  j = j + 1;
end

variable_name = '';

for i = 2:numel(segments)
  segment = segments{i};
  segment = strrep(segment, '//', '/');
  segment = strrep(segment, '&', '&amp;');
  if regexp(segment, '\s')
    segment = ['&quot;' segment '&quot;'];
  end

  if isempty(variable_name)
    variable_name = segment;
  else
    variable_name = [variable_name '.' segment];
  end
end

end
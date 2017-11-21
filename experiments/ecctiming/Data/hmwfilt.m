function [ret] = hmwfilt(datasheet, column, value)
	idx = datasheet(:,column) == value;
	ret = datasheet(idx);
endfunction

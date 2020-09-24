function [status] = CloseTSO()
global 	TSOFID;

% Open threat simiulation output file
status = fclose(TSOFID);

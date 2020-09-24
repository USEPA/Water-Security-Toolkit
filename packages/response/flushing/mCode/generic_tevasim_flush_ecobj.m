function injloctotec = generic_tevasim_flush_ecobj(dem,options)

% States Global Variables to include with the code
global COUNT

fid = fopen('flush.dvf','w');
fprintf(fid,'%d\n',options.debug);
fprintf(fid,'%d %d %d %f %d %d\n',options.det_time, options.resp_delay, ...
    options.valve_delay, options.flush_rate, options.flush_length, ...
    options.sim_duration);

DNODES = options.fl(dem(1:options.nnodes)>0.5);
nn = length(DNODES);
fprintf(fid,'%d ',nn);
for i = 1:length(DNODES)
    fprintf(fid,'%d ',DNODES(i));
end
fprintf(fid,'\n');

if isempty(options.cl)
    DPIPES = [];
else
DPIPES = options.cl(dem(options.nnodes+1:end)>0.5);
end
np = length(DPIPES);
fprintf(fid,'%d ',np);
for i = 1:length(DPIPES)
    fprintf(fid,'%d ',DPIPES(i));
end
fprintf(fid,'\n');
fclose(fid);

if options.need_tsg,
    fid = fopen('flush.tsg','w');
    for i = 1:length(options.inj_locs),
        fprintf(fid,'%s %s %12d %12d %12d\n', options.inj_locs{i}, ...
            options.inj_src_type, options.inj_amt, options.inj_start, ...
            options.inj_stop);
    end
    fclose(fid);
end

eval(['!tevasim.exe --dvf=flush.dvf --tso=flush.tso --tsg=flush.tsg '...
    ' --tsi=flush.tsi ', options.inp_file, ' ', options.rpt_file, ...
    ' > console.out']);
eval(['!tso2Impact --', options.metric, ' flush flush.tso >>console.out']);

fid = fopen(['flush_',options.metric,'.impact'],'r');
impacts = textscan(fid,'%f%f%f%f','Headerlines',2,'CollectOutput',true);
impacts = impacts{1};
fclose(fid);
injloctotec = mean(impacts(impacts(:,2)==-1,4)) + ...
    nn*options.loc_flush_penalty + np*options.loc_close_penalty;
COUNT = COUNT + 1;

if (np+nn)>(options.loc_flush_max+options.loc_close_max),
    injloctotec = injloctotec + Inf;
end

if options.debug
    fprintf(1,'Iter = %-5d;  f(x) = %-15.2f;  n(nd) = %-2d;  n(pp) = %-2d;\n',...
        COUNT, injloctotec, nn, np);
end

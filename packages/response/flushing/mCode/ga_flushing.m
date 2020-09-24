function [x,fval,exitflag,output,population,score] = ga_flushing(metric,flushRate,injlocs)
% This is an auto generated M-file from Optimization Tool.
global COUNT
if nargin > 1,
    tevaopts = get_default_tevaopts();
    if nargin < 3,
        injlocs = {};
    end
    if ~isempty(metric),
        tevaopts.metric = metric;
    end
    if ~isempty(flushRate),
        tevaopts.flush_rate = flushRate;
    end
    if ~isempty(injlocs),
        tevaopts.inj_locs = injlocs;
    end
else
    tevaopts = metric;
end


%FLUSHABLE and CLOSEABLE contain the Node and Link index (col 1) and ID
%(col 2) of the nodes that can be flushed and links that can be closed.
COUNT = 0;
FLUSHABLE=xlsread('Constraints.xls','FLUSHABLE');
if ~isempty(FLUSHABLE)
    FL=FLUSHABLE(:,1)';
else
    FL = [];
end
CLOSEABLE=xlsread('Constraints.xls','CLOSEABLE');
if ~isempty(CLOSEABLE)
    CL=CLOSEABLE(:,1)';
else
    CL = [];
end

NNODES=length(FL);
NLINKS=length(CL);

%Linear constraint - maximum number of decisions made
%A is associated with linear constraints - A*x<=b
%For this case, A is the multiplier on the decision variable - which is 1
%b is the maximum number of decisions made (max. num. of hydrants flushed)
bineq = tevaopts.loc_flush_max + tevaopts.loc_close_max;
Aineq=ones(1,NNODES+NLINKS);

%LB & UB = Lower and upper bounds on variables
%For decision variables, lower bound is 0 (do not change demand)
%and the upper bound is 1 (change demand)
lb=zeros(1,NNODES+NLINKS);
ub=ones(1,NNODES+NLINKS);
nvars = NNODES+NLINKS;

tevaopts.nnodes = NNODES;
tevaopts.nlinks = NLINKS;
tevaopts.fl = FL;
tevaopts.cl = CL;


% Start with the default options
options = gaoptimset;
% Modify options setting
options = gaoptimset(options,'PopulationType', 'doubleVector');
options = gaoptimset(options,'PlotFcns', {  @gaplotbestf @gaplotexpectation });
intPop = tevasiminitialpop(bineq,NNODES,NLINKS)';
options = gaoptimset(options,'PopulationSize', 50);
options = gaoptimset(options,'InitialPopulation', intPop);
options = gaoptimset(options,'MutationFcn', @mutationadaptfeasible);
options = gaoptimset(options,'MigrationDirection', 'both');
options = gaoptimset(options,'Display', 'iter');
options = gaoptimset(options,'OutputFcns', { [] });
options = gaoptimset(options,'Vectorized', 'off');
%options = gaoptimset(options,'InitialPopulation', IntPop);

if isempty(tevaopts.inj_locs),
    fprintf(2,'You MUST provides some injection locations to run this GA!\n');
    return;
end

[x,fval,exitflag,output,population,score] = ...
    ga({@generic_tevasim_flush_ecobj,tevaopts},nvars,Aineq,bineq,[],[],lb,ub,[],options);
generic_tevasim_flush_ecobj(x,tevaopts);
fprintf(1,' Flush: ');
disp((FL(x(1:NNODES)>0.5)));
if isempty(FL(x(1:NNODES)>0.5))
    fprintf(1,'\n');
end
fprintf(1,' Close: ');
disp((CL(x(NNODES+1:end)>0.5)));
if isempty(CL(x(NNODES+1:end)>0.5))
    fprintf(1,'\n');
end
end

function init = tevasiminitialpop(bineq,nNodes,nLinks)
init = zeros(nNodes+nLinks,50);
for i = 1:50
    nIds = unique(ceil(rand(bineq,1)*(nNodes+nLinks)));
    init(nIds,i) = 1;
end
end

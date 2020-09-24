function [linkh,vlinkh,junch,tankh,snodeh,Inovert,Ivert,Ijunc,Itank] = ...
    NetworkFrame(linkh,vlinkh,junch,tankh,snodeh,Inovert,Ivert,Ijunc,Itank,...
    c,logtrans,vmin,vmax,noxy,lwidth,vsize,tsize,...
    plotnodes,legend,v,vx,vy,from,to,nvert,vertx,verty,ivs,vsmarker,...
    vsmarkersize,vsmarkercolor)

% NetworkFrame plots a static frame of the network graph with nodes and links
% colored by v(1)...v(NNODES) using colormap c
% linkh, vlinkh
%           line handles from previous NetworkFrame call, if only the
%           colors are to be changed.  Set to [] on first call or when
%           line objects need to be rendered initially.
% Inovert,Ivert
%           Index arrays defining which links are to be rendered as lines
%           and which as polylines, from previous NetworkFrame call.  
%           Set to [] on first call or when line objects need to be 
%           rendered initially.
% c         colormap
% logtrans  'y' or 'n' to log10 transform the data in v
% vmin,vmax Minimum and maximum values of v that will be plotted - others
%           are ignored
% noxy      do not draw nodes where vx,vy==noxy
% lwidth    link line width
% vsize     vertex size
% tsize     tank vertex size
% plotnodes 'y' or 'n' to plot nodes along with links
% legend    'y' or 'n' to add a legend
% v         vertex values used to color nodes/links
% from,to   lists of from/to node indices for each link
% vx,vy     x,y coordinates of vertices v
% nvert     Array containing number of vertices for each link
% vertx,verty
%           Arrays of link vertex coordinates.  vertx(1:nvert(i),:) and
%           verty(1:nvert(i),:) hold the x,y vertices, respectively, for
%           link i.
% ivs,vsmarker,vsmarkersize,vsmarkercolor
%           Describe node indices (ivs), marker symbols (vmarker),
%           and symbol sizes (vmarkersize) used to annotate the graph
%
% Jim Uber
% 8/15/2005

% Input checking
[m,n]=size(v);
if [m,n]~=size(vx) | [m,n]~=size(vy)
    disp 'v, vx, vy must be the same size'
    return
end
if min(m,n)~=1
    disp 'v, vx, vy must be vectors'
    return
end
if m>n
    v=v';
    vx=vx';
    vy=vy';
end
[m,n]=size(from);
if [m,n]~=size(to)
    disp 'from and to must be the same size'
    return
end
if min(m,n)~=1
    disp 'from and to must be vectors'
    return
end
if m>n
    from=from';
    to=to';
end
nlinks = length(from);
nnodes = length(v);

% Indices of nodes to color
Ivcolor = v>=vmin & v<=vmax;

% Transformation
if logtrans == 'y'
    v(Ivcolor) = log10(v(Ivcolor));
    vmin = log10(vmin);
    vmax = log10(vmax);
end

% Colormap
colormap(c);
cmap = colormap;
[m,n] = size(cmap);                  % m is the number of colors in map
dv = (vmax-vmin)/m;                  % Divide interval into m bins


% LINKS

% Determine link colors
vavg = (v(from) + v(to))/2;          % Average value of adjacent nodes
if dv > 0                            % Select the bin index where v falls
    ic = max(ceil((vavg-vmin)/dv),1);
    ic = min(ic,m);                  % Quick fix for some rounding errors
else
    ic = ones(1,length(v));
end

rgb = cmap(ic,:);                    % Return the color from the map
Iblkline = Ivcolor(from)==0 | Ivcolor(to)==0;
rgb(Iblkline,:) = rgb(Iblkline,:)*0; % Links are black if either node has no color

if length(linkh)==0
    % End point coordinates
    x(1,:) = vx(from);
    x(2,:) = vx(to);
    y(1,:) = vy(from);
    y(2,:) = vy(to);
    % Only plot those lines with valid coordinates
    Idraw = x(1,:) ~= noxy & x(2,:) ~= noxy & y(1,:) ~= noxy & y(2,:) ~= noxy;
    % Plot straight lines without vertices
    Inovert = Idraw & nvert==0;
    linkh=line(x(:,Inovert),y(:,Inovert));
    set(linkh,'LineWidth',lwidth);
end
rgb1=rgb(Inovert,:);
for i=1:length(rgb1(:,1))
    set(linkh(i),{'Color'},{rgb1(i,:)});
end

% Plot lines with vertices
if max(nvert)>0
    n=0;
    if length(vlinkh)==0
        Ivert = Idraw & nvert>0;
        for i=1:nlinks
            if Ivert(i)>0
                n=n+1;
                x1=[x(1,i) full(vertx(i,1:nvert(i))) x(2,i)];
                y1=[y(1,i) full(verty(i,1:nvert(i))) y(2,i)];
                vlinkh(n)=line(x1,y1);
                set(vlinkh(n),'LineWidth',lwidth);
                set(vlinkh(n),{'Color'},{rgb(i,:)});
            end
        end
    else
        for i=1:nlinks
            if Ivert(i)>0
                n=n+1;
                set(vlinkh(n),{'Color'},{rgb(i,:)});
            end
        end
    end
end

% JUNCTIONS (Need to make this more efficient for repeat calls)

if plotnodes=='y'
    if dv > 0                            % Select the bin index where v falls
        ic = max(ceil((v-vmin)/dv),1);
        ic = min(ic,m);                  % Quick fix for some rounding errors
    else
        ic = ones(1,length(v));
    end
    rgb = cmap(ic,:);                    % Return the color from the map
    rgb(~Ivcolor,:) = rgb(~Ivcolor,:)*0; % nodes are black if no color

    if length(junch)==0
        clear x y;
        x(1,:) = vx;                         % X,Y coordinates for each node
        x(2,:) = vx;
        y(1,:) = vy;
        y(2,:) = vy;
        Inode = vx ~= noxy & vy ~= noxy;     % Identify nodes to plot (have coordinates)
        for i=1:nnodes                       % Identify tank nodes
            Itank(i) = istank(i);
        end
        Ijunc = Inode & ~Itank;              % Non tank nodes
        Itank = Inode & Itank;               % tank nodes
        junch=line(x(:,Ijunc),y(:,Ijunc),'Marker','o','MarkerSize',vsize);
        tankh=line(x(:,Itank),y(:,Itank),'Marker','s','MarkerSize',tsize);
    end
    nn=0;
    nt=0;
    for i=1:nnodes
        if Ijunc(i)>0
            nn=nn+1;
            set(junch(nn),{'MarkerFaceColor'},{rgb(i,:)});
            set(junch(nn),{'MarkerEdgeColor'},{rgb(i,:)});
        end
        if Itank(i)>0
            nt=nt+1;
            set(tankh(nt),{'MarkerFaceColor'},{rgb(i,:)});
            set(tankh(nt),{'MarkerEdgeColor'},{rgb(i,:)});
        end
    end
end

% Add colorbar legend
if legend=='y'
    hc = colorbar;
    [mapsize,n] = size(cmap);
    ytick = [1: floor(mapsize/4) : mapsize];   % ytick holds the colormap positions to label
    [m,ticksize] = size(ytick);
    if ytick(ticksize) ~= mapsize
        ticksize = ticksize + 1;
        ytick(ticksize) = mapsize;
    end
    dv = (vmax - vmin)/mapsize;
    for i = 1:ticksize
        if logtrans == 'y'
            labelvalue = 10^(vmin + (ytick(i) - 1)*dv);
        else
            labelvalue = vmin + (ytick(i) - 1)*dv;
        end
        labelstring = num2str( labelvalue, 3 );
        ylab(i) = {labelstring};
    end
    set(hc,'YTick',ytick);
    set(hc,'YTickLabel',ylab');
end

% Extra node symbols
if length(ivs)>0
    if length(snodeh)==0
        clear x y;
        x(1,:) = vx; 
        x(2,:) = vx;
        y(1,:) = vy;
        y(2,:) = vy;
        for i=1:length(ivs)
            snodeh(i)=line(x(:,ivs(i)),y(:,ivs(i)),'Marker',vsmarker(i),...
                'MarkerSize',vsmarkersize(i),...
                'MarkerFaceColor',vsmarkercolor(i),...
                'MarkerEdgeColor',vsmarkercolor(i));
        end
    else
        for i=1:length(ivs)
            set(snodeh(i),'Marker',vsmarker(i));
            set(snodeh(i),'MarkerSize',vsmarkersize(i));
            set(snodeh(i),'MarkerFaceColor',vsmarkercolor(i));
            set(snodeh(i),'MarkerEdgeColor',vsmarkercolor(i));
        end
    end
end

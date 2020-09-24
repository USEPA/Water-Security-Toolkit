function [] = NetworkMovie(fig,movfname,quality,fps,c,speciename,...
    logtrans,vmin,vmax,lwidth,vsize,tsize,plotnodes,tframe,vsnodeid,...
    vsmarker,vsmarkersize,vsmarkercolor)

% NetworkMovie plots the network graph on figure handle fig with nodes and links
% colored by C(:,i) using colormap c, and animates the images
% at different time steps i.
%
% fig         Existing figure handle to use.  If [] then new figure is created.
% movfname    AVI filename to store movie.  If [] then no movie is made,
%             but the animation is displayed.
% quality     0-100 quality of AVI movie
% fps         frames per second of AVI movie
% c           colormap
% speciename  specie ID to plot
% logtrans    'y' or 'n' to log10 transform the data
% vmin        Ignore (do not color) values v<vmin
% vmax        Ignore (do not color) values v>vmax
%             vmin, vmax define the limits of the color mapping, e.g.  
%             vmin = min(min(C(:,t1:t2)));
%             vmax = max(max(C(:,t1:t2)));
%             vmin = max(vmin,vlow); 
%             where vlow is a lower bound (useful to avoid log10(<=0)
% lwidth      link line width
% vsize       vertex size
% tsize       tank vertex size
% plotnodes   'y' or 'n' depending on whether nodes should be plotted in 
%             addition to links
% tframe      start and stop time steps for the movie (or [] to use all
%             steps)
% vsnodeid,vsmarker,vsmarkersize,vsmarkercolor
%             Specify special symbols and attributes to plot on node IDs
%             listed in cell array vsnodeid.  May all be [].
%
% Jim Uber
% 8/15/2005

setglobals;

is = SpecieIndex(speciename);
if is<1 | is>NSPECIES
    disp 'Invalid specie name'
    return
end
if length(tframe)==2
    if tframe(1)<1 | tframe(2)>length(C(1,:)) | tframe(1)>tframe(2)
        disp 'Invalid movie time frame'
        return
    end
    t1=tframe(1);
    t2=tframe(2);
else
    t1 = 1;
    t2 = length(C(1,:,is));
end
ivs=[];
for i=1:length(vsnodeid)
    ivs(i)=NodeIndex(vsnodeid(i));
    if ivs(i)<1 | ivs(i)>NNODES
        disp 'Invalid symbol node ID'
        return
    end
end
makeavi = length(movfname)>0;

% Figure props
if length(fig)==0
    fig=figure;
    axis equal;
    axis off;
%    set(fig,'Color',[0 0 0]);
else
    figure(fig);
end
if makeavi
    mov = avifile(movfname,'quality',quality,'fps',fps,'compression','Cinepak');
end

% Draw the first frame and stop to allow user adjustment of figure window
[linkh,vlinkh,junch,tankh,snodeh,Inovert,Ivert,Ijunc,Itank]=...
    NetworkFrame([],[],[],[],[],[],[],[],[],c,logtrans,...
    vmin,vmax,FLTMAX,lwidth,vsize,tsize,plotnodes,'y',C(:,t1,is),...
    XNODE,YNODE,FROMNODE,TONODE,NV,VERTX,VERTY,ivs,vsmarker,vsmarkersize,vsmarkercolor);
disp 'Adjust figure window properties as desired, and press any key to continue'
pause;
if makeavi
    M=getframe(fig);
    mov = addframe(mov,M);
else
    drawnow;
end
fprintf(1,'Frame: %d\n',t1);

% Draw the rest
for i=t1+1:t2
    [linkh,vlinkh,junch,tankh,snodeh,Inovert,Ivert,Ijunc,Itank]=...
        NetworkFrame(linkh,vlinkh,junch,tankh,snodeh,...
        Inovert,Ivert,Ijunc,Itank,c,logtrans,vmin,vmax,FLTMAX,lwidth,vsize,tsize,...
        plotnodes,'n',C(:,i,is),XNODE,YNODE,FROMNODE,TONODE,NV,VERTX,VERTY,...
        ivs,vsmarker,vsmarkersize,vsmarkercolor);
    if makeavi
        M=getframe(fig);
        mov = addframe(mov,M);
    else
        drawnow;
    end
    fprintf(1,'Frame: %d\n',i);
end

if makeavi
    mov = close(mov);
end

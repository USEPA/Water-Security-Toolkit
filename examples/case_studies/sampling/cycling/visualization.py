import matplotlib.pyplot as plt
import pywst.common.wst_util as wst_util
from util import declare_tempfile
from util import remove_tempfiles
import pyutilib.subprocess
import numpy as np
import yaml
import pyutilib
import matplotlib


def plot_histogram(filename,data,**kwds):

    multi_data = False
    if len(data):
        if isinstance(data[0],list):
            multi_data = True
    
    title = kwds.pop('title','')
    xlabel = kwds.pop('xlabel','Value')
    ylabel = kwds.pop('ylabel','Frequency')
    colors = matplotlib.colors.cnames.keys()
    if multi_data:
        for i,d in enumerate(data):
            plt.hist(d,color=colors[i],label=colors[i],**kwds)
            plt.legend()
    else:
        plt.hist(data,**kwds)

    plt.savefig(filename)
    plt.figure()

def probability_map(inpfile,outfile, dict_node_probs,sampled_nodes=None,parallel=False,**kwds):

    screen_size = kwds.pop('screen size', [1200, 800])
    lengend_location = kwds.pop('legend location', [800, 20])
    cycle = kwds.pop('cycle','')

    # red nodes
    red_nodes = list(dict_node_probs['red'])
    r_nodes = zip(*red_nodes)
    # yellow nodes
    yellow_nodes = list(dict_node_probs['yellow'])
    y_nodes = zip(*yellow_nodes)
    # green nodes
    green_nodes = list(dict_node_probs['green'])
    g_nodes = zip(*green_nodes)
    template_options = {
	'network': {'epanet file': inpfile},
	'visualization': { 'screen': { 'size': screen_size},
		           'legend': { 'location': lengend_location},
		           'layers': [ { 'label': 'Contaminated Nodes',
		                         'location type': 'node',
		                         'locations': [] if not r_nodes else list(r_nodes[0]),
		                         'shape': 'circle',
		                         'fill':{ 'color': 'red'}
                                         ,},
                                       { 'label': 'Uncertain Nodes',
		                         'location type': 'node',
		                         'locations': [] if not y_nodes else list(y_nodes[0]),                 
		                         'shape': 'circle',
		                         'fill':{ 'color': 'yellow'}
                                         },
                                       { 'label': 'Clean Nodes',
		                         'location type': 'node',
		                         'locations': [] if not g_nodes else list(g_nodes[0]),
		                         'shape': 'circle',
		                         'fill':{ 'color': '#a0b0f8'}
                                         ,}
                           ]},
	'configure': {'output prefix': outfile}}

    if sampled_nodes is not None:
        template_options['visualization']['layers'].append( { 'label': 'Grab Sample Locations',
		                                              'location type': 'node',
		                                              'locations': sampled_nodes,                 
		                                              'shape': 'diamond',
		                                              'fill':{ 'color': 'black'}})

    
    #yaml_file = tempfile.NamedTemporaryFile(prefix='visual_',suffix='.yml')
    yaml_file = open("visual_{}.yml".format(outfile),'w')
    yaml.dump(template_options,yaml_file,default_flow_style=False)
    cmd = "wst visualization {}".format(yaml_file.name)
    yaml_file.close()
    """
    if not parallel:
        p = pyutilib.subprocess.run(cmd)
        if (p[0]):
            print 'An error occured when creating visualization map\n'
        remove_tempfiles()
    """

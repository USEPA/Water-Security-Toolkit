Water Security Toolkit (WST)
=======================================

WST is a software package which integrates water distribution network simulation 
and optimization to support the design of contamination warning systems and response action plans. 
Capabilities in WST include:

* Hydraulic and water quality simulation
* Public health and network impact assessment
* Sensor placement optimization
* Flushing optimization
* Booster placement optimization
* Manual sampling optimization
* Source inversion

WST was developed between 2011 and 2018.  The software includes the impact assessment and sensor 
placement optimization methods that were originally released in TEVA-SPOT. WST was developed 
through a collaborative effort between the U. S. Environmental Protection Agency, Sandia 
National Laboratories, Argonne National Laboratory, University of Cincinnati, and Texas A&M University.
WST is no longer under active development.  Future development for water 
security applications will be included in [WNTR](https://github.com/USEPA/WNTR) or in related Python packages.

User Manual
-------------
The WST user manual can be downloaded from ...

License
---------
WST is released under the Revised BSD license. See the LICENSE.txt file.

Install
---------
Windows installation instructions are in the [install](INSTALL.md) file.

References
---------------
The methods in WST have been published in several journal articles, including:

* Berry, J., Hart, W., Phillips, C., Uber, J., and Watson, J. (2006). "Sensor Placement in Municipal Water Networks with Temporal Integer Programming Models." J. Water Resour. Plann. Manage., 10.1061/(ASCE)0733-9496(2006)132:4(218), 218-224. 
* Carr, R.D., H.J. Greenberg, W.E. Hart, G. Konjevod, E. Lauer, H. Lin, T. Morrison, C.A. Phillips, 2006, Robust optimization of contaminant sensor placement for community water systems, Mathematical Programming, 107(1), 337-356
* Laird, C., Biegler, L., and van Bloemen Waanders, B. (2006). Mixed-Integer Approach for Obtaining Unique Solutions in Source Inversion of Water Networks J. Water Resour. Plann. Manage., 4(242), 242-251
* Ostfeld, A., Uber, J., Salomons, E., Berry, J., Hart, W., Phillips, C., Watson, J., Dorini, G., Jonkergouw, P., Kapelan, Z., di Pierro, F., Khu, S., Savic, D., Eliades, D., Polycarpou, M., Ghimire, S., Barkdoll, B., Gueli, R., Huang, J., McBean, E., James, W., Krause, A., Leskovec, J., Isovitsch, S., Xu, J., Guestrin, C., VanBriesen, J., Small, M., Fischbeck, P., Preis, A., Propato, M., Piller, O., Trachtman, G., Wu, Z., and Walski, T. (2008). "The Battle of the Water Sensor Networks (BWSN): A Design Challenge for Engineers and Algorithms." J. Water Resour. Plann. Manage., 6(556), 556-568
* Hart, W.E., J.W. Berry, E. Boman, C.A. Phillips, L. A. Riesen, and J.P. Watson (2008), Limited-Memory Techniques for Sensor Placement in Water Distribution Networks, Proceedings of the Learning and Intelligent Optimization conference, Springer Volume 5313, 125-137.
* Berry, J., Boman, E., Phillips, C., and Riesen, L. (2008) Low-Memory Lagrangian Relaxation Methods for Sensor Placement in Municipal Water Networks. World Environmental and Water Resources Congress 2008: pp. 1-10. 
* Hart, W., Berry, J., Boman, E., Murray, R., Phillips, C., Riesen, L., and Watson, J. (2008) The TEVA-SPOT Toolkit for Drinking Water Contaminant Warning System Design. World Environmental and Water Resources Congress 2008: pp. 1-12. 
* R. Murray, W. E. Hart, C. A. Phillips, J. Berry, E. Boman, R. D. Carr, L. A. Riesen, J.-P. Watson, T. Baranowski, G. Gray, J. Herrmann, R. Janke, T. N. Taxon, J. Uber, K. Morley, (2009), “U. S. Environmental Protection Agency uses Operations Research to Reduce Drinking Water Contamination Risks,” Edelman finalist paper, Interfaces, Vol. 39, No. 1, pp. 57-68 
* Berry, J., Carr, R., Hart, W., Leung, V., Phillips, C., and Watson, J. (2009). "Designing Contamination Warning Systems for Municipal Water Networks Using Imperfect Sensors." J. Water Resour. Plann. Manage., 10.1061/(ASCE)0733-9496(2009)135:4(253), 253-263. 
* R. Murray, T. Haxton, R. Janke, W. E. Hart, J. W. Berry, and C. A. Phillips (2010). “Sensor Network Design for Contamination Warning Systems: A Compendium of Research Results and Case Studies Using the TEVA-SPOT Software.” U. S. Environmental Protection Agency, Office of Research and Development, National Homeland Security Research Center, Cincinnati OH. EPA/600/R-09/141.
* R. Murray, T. Haxton, W. E. Hart, C. A. Phillips (2011), “Real-world case studies for sensor network design of drinking water contamination warning systems,” Handbook of Water and Wastewater Systems Protection, editors: R. M. Clark, S. Hakim, and A. Ostfeld, Series: Protecting Critical Infrastructure, Springer, New York, 2011, pp. 319-348. 
* Hart, W., Murray, R., and Phillips, C. (2011) Minimize Impact or Maximize Benefit: The Role of Objective Function in Approximately Optimizing Sensor Placement for Municipal Water Distribution Networks. World Environmental and Water Resources Congress 2011: pp. 330-339.
* Klise, K., Phillips, C., and Janke, R. (2013). "Two-Tiered Sensor Placement for Large Water Distribution Network Models." J. Infrastruct. Syst., 10.1061/(ASCE)IS.1943-555X.0000156, 465-473.
* Mann, A., Hackebeil, G., and Laird, C. (2014). "Explicit Water Quality Model Generation and Rapid Multiscenario Simulation." J. Water Resour. Plann. Manage., 10.1061/(ASCE)WR.1943-5452.0000278, 666-677. 
* Seth, A., Klise, K., Siirola, J., Haxton, T., and Laird, C. (2016). "Testing Contamination Source Identification Methods for Water Distribution Networks." J. Water Resour. Plann. Manage., 10.1061/(ASCE)WR.1943-5452.0000619, 04016001. 
* A Seth, GA Hackebeil, KA Klise, T Haxton, R Murray, CD Laird (2017), "Efficient reduction of optimal disinfectant booster station placement formulations for security of large-scale water distribution networks", Engineering Optimization 49 (8), 1281-1298.

Contact
--------

   * Terra Haxton, US Environmental Protection Agency, Haxton.Terra@epa.gov

EPA Disclaimer
-----------------

The United States Environmental Protection Agency (EPA) GitHub project code is provided on an "as is" 
basis and the user assumes responsibility for its use. EPA has relinquished control of the information and 
no longer has responsibility to protect the integrity , confidentiality, or availability of the information. Any 
reference to specific commercial products, processes, or services by service mark, trademark, manufacturer, 
or otherwise, does not constitute or imply their endorsement, recommendation or favoring by EPA. The EPA 
seal and logo shall not be used in any manner to imply endorsement of any commercial product or activity 
by EPA or the United States Government.

Sandia Disclaimer
--------------------------------

Sandia National Laboratories is a multimission laboratory managed and operated by National Technology and 
Engineering Solutions of Sandia, LLC., a wholly owned subsidiary of Honeywell International, Inc., for the 
U.S. Department of Energy's National Nuclear Security Administration under contract DE-NA-0003525.

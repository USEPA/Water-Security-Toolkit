#include "epanetbtx.h"
#include "btxtypes.h"
#include "btxfuns.h"
#include "epanet2.h"
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

extern BTXproject BTX;
/*Particle backtracking*/
int backtrack()
{
	float criteria = 1000.0, tank_flow, current_volume, erase_sum=0, tankinflow, tankoutflow;
	int ntanks = BTX.Network.ntanks, nlinks = BTX.Network.nlinks, tankindex, i;
	int ninlink; 
	int inlink[20]; 
	float current_time, flow[20], distance[20];
	struct Sparticle *element, *temp;

	while(0.001*criteria > TOTALCRI)
	{	
		/*if tank is filling in the hydraulic period, generate particles in the tank inlet*/
		for(tankindex = 1; tankindex <= ntanks; tankindex++)
		{
			tankinflow = inflow(&ninlink, inlink, flow, distance, BTX.Tank[tankindex-1].nodeindex);
			tankoutflow = outflow(BTX.Tank[tankindex-1].nodeindex);
			tank_flow = tankinflow+tankoutflow;

			//if the tank is filling and tank impact is not negligible
			if(BTX.TankImpactCoeff[tankindex-1]>0)
			{
				current_time = BTX.Time;
				current_volume = BTX.TankVolume[tankindex-1][BTX.Nstep];
				for(i=1; i<=(int)((BTX.Time - BTX.SaveTime[BTX.Nstep-1])/QUALITYSTEP); i++)
				{
					generate_particle_in_tank(&current_volume, tankinflow, tankoutflow, tank_flow, &current_time, tankindex);
				}
			}
//			BTX.TimeofTankImpact[tankindex-1] = BTX.SaveTime[BTX.Nstep-1];
		}

		element = BTX.ParticleFirst;
	
		while(element!=NULL)
		{
			temp = element->next;
			if(element->a >= 1000.0*BTX.Erasecri)
				moveparticle(element);
			else
			{
				erase_sum += element->a;
				eraseparticle(element);
			}
			element = temp;
		}

		BTX.Time = (float)BTX.SaveTime[BTX.Nstep-1];
		BTX.Nstep -= 1;

		for (tankindex = 1; tankindex <= ntanks; tankindex++)
		{
			if (BTX.TimeofTankImpact[tankindex-1] > BTX.Time && BTX.Tank[tankindex-1].mixtype <= MIX2)		
			{
				BTX.TankImpactCoeff[tankindex-1] = BTX.TankImpactCoeff[tankindex-1]*(float)exp(BTX.Tank[tankindex-1].decay_coeff/(24*3600.0)*(BTX.TimeofTankImpact[tankindex-1]-BTX.Time));
				BTX.TimeofTankImpact[tankindex-1] = BTX.Time;
			}
		
		}

		criteria = 0;
		temp = BTX.ParticleFirst;
		while(temp!=NULL)
		{	
			criteria += temp->a;
			temp = temp->next;
		}

		for(i = 0; i < ntanks; i++)
		{
			criteria += BTX.TankImpactCoeff[i];
			criteria += BTX.TankMainImpactCoeff[i];
		}
		if(BTX.Nstep <= 0)
		{	
			return(ERR_BACKTRACK_TIME); /*Reach the beginning of the simulation*/
		}
	}
	return (0);
}

/*tank is draining*/
void updatetankimpact(int i, float tankflow)
{
	float mainpartvolume, inpartvolume, tempvalue, tempvalue2, current_volume;
	int j;
	float k_tank = (float)BTX.Tank[i].decay_coeff/(24*3600);

	if (BTX.Tank[i].mixtype == MIXED)
	{
		tempvalue = BTX.TankImpactCoeff[i]*(float)exp(k_tank*(BTX.TimeofTankImpact[i]-BTX.Time));
		BTX.TankImpactCoeff[i] = tempvalue;
	}
	else
	{
		current_volume = BTX.TankVolume[i][BTX.Nstep];
		for(j = 1; j <= (int)((BTX.TimeofTankImpact[i]-BTX.Time)/QUALITYSTEP); j++)
		{
			if (current_volume <= BTX.Tank[i].mixvolume)
			{
				tempvalue = BTX.TankImpactCoeff[i]*exp(k_tank*QUALITYSTEP);
				BTX.TankImpactCoeff[i] = tempvalue;
			}
			else
			{
				inpartvolume = BTX.Tank[i].mixvolume;
				mainpartvolume = current_volume - inpartvolume;
				tempvalue = BTX.TankMainImpactCoeff[i]*exp(k_tank*QUALITYSTEP);
				if(k_tank != 0)
					tempvalue += BTX.TankImpactCoeff[i]*tankflow*(1-exp(k_tank*QUALITYSTEP))/(-k_tank*(inpartvolume));
				else
					tempvalue += BTX.TankImpactCoeff[i]*tankflow*QUALITYSTEP/inpartvolume;
				BTX.TankMainImpactCoeff[i] = tempvalue;

				tempvalue2 = BTX.TankImpactCoeff[i];
				BTX.TankImpactCoeff[i] = tempvalue2*(inpartvolume-tankflow*QUALITYSTEP)/(inpartvolume+0)*exp(k_tank*QUALITYSTEP);
			}
			current_volume += tankflow*QUALITYSTEP; 
		}
	}
}

/*tank is filling*/
int generate_particle_in_tank(float *current_volume, float tankinflow, float tankoutflow, float tank_flow, float * current_time, int tankindex)
{
	struct Sparticle * newparticle; 
	float previous_volume, tempvalue, tempvalue2, k_tank, inpartvolume, mainpartvolume;

	k_tank = (float)BTX.Tank[tankindex-1].decay_coeff/(24*3600);
	
	if (BTX.Tank[tankindex-1].mixtype == MIX2 && *current_volume >= BTX.Tank[tankindex-1].mixvolume)
	{
		inpartvolume =  BTX.Tank[tankindex-1].mixvolume;
		mainpartvolume = *current_volume - inpartvolume;
	}
	else
	{
		inpartvolume = *current_volume;
		mainpartvolume = 0;
	}

	//generate first particle
	if (tankinflow > 0)
	{
		BTX.ParticleNumber++;
		if(BTX.ParticleFirst == NULL)
		{
			newparticle = generateparticle();
			if(newparticle == NULL) return(ERR_MEMORY);
			newparticle->next = NULL;
			newparticle->previous = NULL;
			BTX.ParticleFirst = newparticle;
			BTX.ParticleLast = BTX.ParticleFirst;
		}
		else
		{
			BTX.ParticleLast->next = generateparticle();		
			if(BTX.ParticleLast->next == NULL)return (ERR_MEMORY); 
			newparticle = BTX.ParticleLast->next;
			newparticle->previous = BTX.ParticleLast;
			newparticle->next = NULL;
			BTX.ParticleLast = newparticle;
		}
		newparticle->t = *current_time;
		if (k_tank != 0)
			newparticle->a = BTX.TankImpactCoeff[tankindex-1]*tankinflow*(1-exp(k_tank*QUALITYSTEP))/(-k_tank*(inpartvolume));
		else
			newparticle->a = BTX.TankImpactCoeff[tankindex-1]*tankinflow*QUALITYSTEP/(inpartvolume);
		splitparticle(newparticle, BTX.Tank[tankindex-1].nodeindex);	
	}

	previous_volume = *current_volume - tank_flow*QUALITYSTEP;
	
	/*update tank impact coefficient*/

	if (BTX.Tank[tankindex-1].mixtype == MIXED)
	{
		tempvalue = BTX.TankImpactCoeff[tankindex-1]*exp(k_tank*QUALITYSTEP)*(1-tankinflow*QUALITYSTEP/inpartvolume); //lew
		tempvalue2 = 0.0;
	}
	else   //two compartment model
	{
		if (tank_flow > 0) //net filling
		{
			if (previous_volume >= BTX.Tank[tankindex-1].mixvolume) //ambient zone filling
			{
				tempvalue = BTX.TankImpactCoeff[tankindex-1]*(exp(k_tank*QUALITYSTEP)-tankinflow*QUALITYSTEP/inpartvolume)+tank_flow*QUALITYSTEP*BTX.TankMainImpactCoeff[tankindex-1]/mainpartvolume; //feng
			
				if (previous_volume == BTX.Tank[tankindex-1].mixvolume)
					tempvalue2 = 0;
				else
					tempvalue2 = BTX.TankMainImpactCoeff[tankindex-1]*exp(k_tank*QUALITYSTEP)-tank_flow*QUALITYSTEP*BTX.TankMainImpactCoeff[tankindex-1]/mainpartvolume;
			}
			else
			{
				tempvalue = BTX.TankImpactCoeff[tankindex-1]*(exp(k_tank*QUALITYSTEP)-tankinflow*QUALITYSTEP/inpartvolume); //feng
				tempvalue2 = 0.0;
			}
		}
		else if ( tank_flow < 0) //net draining
		{
			if (previous_volume >= BTX.Tank[tankindex-1].mixvolume) //ambient zone draining
			{
				tempvalue = BTX.TankImpactCoeff[tankindex-1]*(exp(k_tank*QUALITYSTEP)+tankoutflow*QUALITYSTEP/inpartvolume); //feng
				tempvalue2 = BTX.TankMainImpactCoeff[tankindex-1]*exp(k_tank*QUALITYSTEP)-BTX.TankImpactCoeff[tankindex-1]*tank_flow*QUALITYSTEP/inpartvolume;
			}
			else
			{
				tempvalue = BTX.TankImpactCoeff[tankindex-1]*exp(k_tank*QUALITYSTEP)-tankinflow*QUALITYSTEP/inpartvolume; //feng
				tempvalue2 = 0.0;
			}	
		}
		else
		{
			tempvalue = BTX.TankImpactCoeff[tankindex-1]*exp(k_tank*QUALITYSTEP);			
			tempvalue2 = BTX.TankMainImpactCoeff[tankindex-1]*exp(k_tank*QUALITYSTEP);

		}
	}

	BTX.TankImpactCoeff[tankindex-1] = tempvalue;
	BTX.TankMainImpactCoeff[tankindex-1] = tempvalue2;
		
	*current_time = *current_time - QUALITYSTEP;
	*current_volume = previous_volume;
	BTX.TimeofTankImpact[tankindex-1] = *current_time;

	return(0);

}


int moveparticle(struct Sparticle *element)
/*
-----------------------------------------------------------------------------------
   Input:   element = backtracking particle              
   Output:  none
   Returns: error code ( 0 for no error)                            
   Purpose: backtrack a single particle under constant hydraulics 
------------------------------------------------------------------------------------
*/
{	
	float current_time, length, diameter, velocity, x, a, flowrate, k, tankinflow, tankoutflow;
    float deltat, tempvalue;  
	int errcode = 0,tankindex, meettank, ntanks = BTX.Network.ntanks, upnodetype;
    int fromnode, tonode, up_node, which_link, ninlink;
	int inlink[20], position;

	float source_concentration=1.0, basedemand; /*assumed constant source concentration*/
	float walldecay;
	float distance[20], flow[20];
	char linkid[80];
	char nodeid[80];

	
	which_link = element->link; /*index of the link where particle stays*/
	
	if (which_link > 0)
	{
		BTX.PathFlag[which_link-1] = 1;

		errcode = ENgetlinkid(which_link, linkid);
		if (errcode) return (errcode);

		/*get link information*/
		errcode = ENgetlinkvalue(which_link, EN_LENGTH, &length); 
		if(errcode) return (errcode);
		if (BTX.LUcf != 1.0)
			length = length/BTX.LUcf;

		errcode = ENgetlinkvalue(which_link, EN_DIAMETER, &diameter);
		if(errcode) return (errcode);

		errcode = ENgetlinknodes(which_link, &fromnode, &tonode);
		if(errcode) return (errcode);
	
		flowrate = readflowrate(which_link, BTX.Nstep);

		if (BTX.LUcf == 1.0)
			diameter = diameter/12.0;/*diameter unit transformation - inch to foot*/
		else
			diameter = diameter*0.001/BTX.LUcf;
	
		/*find upnode*/
		if (flowrate >= 0)
			up_node = fromnode;
		else
			up_node = tonode;
	}
	else	//in plug flow tank
	{
		tankindex = 0 - which_link;
		up_node = BTX.Tank[tankindex-1].nodeindex;
		tankinflow = inflow(&ninlink, inlink, flow, distance, up_node);
		tankoutflow = outflow(up_node);
	}

	/*deltat is the travel time to the upnode under current hydraulic condition*/
	a = element->a; /*particle quality*/
	x = element->x; /*position in the link*/ 
	if (diameter > TINY && which_link > 0)
	{
		velocity = (float)(flowrate*4.0/(PI*diameter*diameter));
		if (Absolute(velocity) <= 0.00001)
		{
			velocity = 0.0;
			deltat = 360000;
		}
		else
		{
			if(velocity >= 0)
				deltat = x/velocity;
			else
				deltat = (length-x)/(-velocity);
		}
		ENgetlinkvalue(which_link, EN_KWALL, &walldecay);
		k = getpiperate(which_link, Absolute(velocity), diameter, length);

	}
	else if (diameter <= TINY && which_link > 0) 
	{
		deltat = 0;
		k = 0;
	}
	else
	{
		k = BTX.Tank[tankindex-1].decay_coeff;
		if ( BTX.Tank[tankindex-1].mixtype == FIFO)
		{
			flowrate = tankinflow;
			if (flowrate > 0.0)
				deltat = element->x/flowrate;
			else
				deltat = 3600000;   //no inflow
		}
		else
		{
			flowrate = tankinflow + tankoutflow;
			if (flowrate > 0.0)
				deltat = element->x/flowrate;
			else
				deltat = 3600000;   //no net flow, particle stay in the same position
		}
	}


	meettank = 0;
	if ( which_link > 0 && velocity == 0)
	{
		if(element->t == BTX.SaveTime[BTX.Nstep-1]) 
		{
			element->a = element->a*exp(k*(BTX.SaveTime[BTX.Nstep-1] - BTX.SaveTime[BTX.Nstep-2])/(24*3600));
			element->t = (float)BTX.SaveTime[BTX.Nstep-2];		
		}
		else
		{
			element->a = element->a*exp(k*(element->t - BTX.SaveTime[BTX.Nstep-1])/(24*3600));
			element->t = (float)BTX.SaveTime[BTX.Nstep-1];
		}

		return (0);
	}
	else if(deltat > (element->t-BTX.SaveTime[BTX.Nstep-1]))/*determine whether the particle will hit upnode before the end of present hydraulic step*/
	{	
		if ( which_link > 0)
		{
			x = x - velocity*(element->t - BTX.SaveTime[BTX.Nstep-1]);		
		}
		else
			x = x - flowrate*(element->t - BTX.SaveTime[BTX.Nstep-1]);

		a = a*(float)exp(k*(element->t - BTX.SaveTime[BTX.Nstep-1])/(24*3600));
		element->x = x;
		element->a = a;
		element->t = (float)BTX.SaveTime[BTX.Nstep-1];
		return(0);
	}
	else	/*reach the upstream node*/
	{
		if (which_link < 0)
			element->x = 0;
		else if (velocity > 0)
			element->x = 0;
		else
			element->x = length;

		a = (float)(a*exp(k*deltat/(24*3600)));
		element->a = a;
		element->t = element->t-deltat;					
		

		if (BTX.Node[up_node].inputflag == 1)
		{

			if (BTX.Node[up_node].sourcetype == EN_CONCEN)	/*setpoint source*/
			{
				position = ((int)(element->t/(3600.0*BTX.Network.impactstep)));
				BTX.Node[up_node].impact[position] += element->a/1000.0;
			}

			else if (BTX.Node[up_node].sourcetype == EN_SETPOINT)	/*setpoint source*/
			{
				position = ((int)(element->t/(3600.0*BTX.Network.impactstep)));
				BTX.Node[up_node].impact[position] += element->a/1000.0;
				errcode = addpath(up_node, (BTX.Outtime - element->t)/3600.0, element->a/1000.0);
				eraseparticle(element);
				return (errcode);
			}
			else if (BTX.Node[up_node].sourcetype == EN_FLOWPACED)
			{
				position = ((int)(element->t/(3600.0*BTX.Network.impactstep)));
				BTX.Node[up_node].impact[position] += element->a/1000.0;
			}
			else if (BTX.Node[up_node].sourcetype == EN_MASS)
			{
				flowrate = flowofmasssource(up_node); /* L/minute */
				position = ((int)(element->t/(3600.0*BTX.Network.impactstep)));
				BTX.Node[up_node].impact[position] += element->a/(flowrate*1000.0);
			}
		}		
		
		errcode = ENgetnodetype(up_node, &upnodetype);
		if (errcode) return (errcode);
		basedemand = 0;
		if (upnodetype == EN_JUNCTION)
		{		
			errcode = ENgetnodevalue(up_node, EN_BASEDEMAND, &basedemand);
			if (errcode) return (errcode);
		}

		if(upnodetype == EN_RESERVOIR ||basedemand < 0) /*reservoire or negtive demand*/
		{
			if(BTX.Node[up_node].inputflag == 0)
				errcode = addpath(up_node, (BTX.Outtime-element->t)/3600.0, 0.0);	
			else if(BTX.Node[up_node].inputflag == 1 && BTX.Node[up_node].sourcetype == EN_MASS)
			{
				flowrate = flowofmasssource(up_node);
				errcode = addpath(up_node, (BTX.Outtime-element->t)/3600.0, element->a/(flowrate*1000.0));
			}
			else
				errcode = addpath(up_node, (BTX.Outtime-element->t)/3600.0, element->a/1000.0);	

			eraseparticle(element);
			return (errcode);
		}
		else if (upnodetype == EN_TANK)  /*tank*/
		{
			for(tankindex = 1; tankindex <= ntanks; tankindex++)
			{
				if(up_node == BTX.Tank[tankindex-1].nodeindex)
				{
					if (BTX.Tank[tankindex-1].mixtype == MIXED ||BTX.Tank[tankindex-1].mixtype == MIX2)
					{
						//Time = element->t;
						current_time = element->t;
						tempvalue = BTX.TankImpactCoeff[tankindex-1]*exp(BTX.Tank[tankindex-1].decay_coeff*(BTX.TimeofTankImpact[tankindex-1] - element->t)/(24*3600));
						BTX.TankImpactCoeff[tankindex-1] = tempvalue;
						particletotank(element,tankindex);	
						meettank = 1;
						return (0);
					}
					else if (BTX.Tank[tankindex-1].mixtype == LIFO)
					{
						if (tankindex != (0- which_link))
						{
							element->x = 0;
							element->link = 0-tankindex;
							moveparticle(element);
						}
						else
						{
							if (splitparticle(element, up_node) == 0)
								moveparticle(element);
							else
								return 0;
						}
					}
					else
					{
						if ( tankindex != (0-which_link))
						{
							tankinflow = inflow(&ninlink, inlink, flow, distance, BTX.Tank[tankindex-1].nodeindex);
							tankoutflow = outflow(BTX.Tank[tankindex-1].nodeindex);
							element-> x = BTX.TankVolume[tankindex-1][BTX.Nstep-1]+(tankinflow+tankoutflow)*(element->t - BTX.SaveTime[BTX.Nstep-1]);
							element->link = 0-tankindex;
							moveparticle(element);
						}		
						else
						{
							if (splitparticle(element, up_node) == 0)
								moveparticle(element);
							else
								return 0;												
						}
					}
				}
			}
		}
		else			//normal junctions with positive demand 
		{	
			errcode = ENgetnodeid(up_node, nodeid);
			/*generate new particle if more than one inflow, add the new ones to the end of link list*/ 
			if (splitparticle(element, up_node) == 0)
				moveparticle(element); /*recursion, move the parent particle till the end of the hydraulic step*/
			else
				return(0);	
		}
	}
	return (errcode);
}


/*inflow rate to the masssource or outflow of reservoir or tank*/
float flowofmasssource(int inputindex) 
{
	struct Sadj * tempadj;
	int linkindex, flag;
	float inflow, outflow, tempflow;
	inflow = 0.0, outflow = 0.0;
//	tempadj = *(ADJ + inputindex - 1);
	tempadj = BTX.ADJ[inputindex-1];
	/*check adjacent nodes and links*/
	while(tempadj != NULL)
	{
		linkindex = tempadj->linkindex;
		flag = tempadj->flag;		
		tempflow = readflowrate(linkindex, BTX.Nstep);
		tempflow = tempflow*60*LPSperCFS; /*ft3 per second to lpm*/

		if(tempflow*flag < 0)/*inflow link*/
			inflow += Absolute(tempflow);		
		else /*outflow link*/
			outflow += Absolute(tempflow);
		
		tempadj = tempadj->next; /*check the next adjacent node*/
	}

	if (inflow != 0.0)
		return (inflow);
	else 
		return (outflow);
}

int particletotank(struct Sparticle * particle, int tankindex)
{
	struct Sparticle *temp1, *temp2;
	
	temp1=particle->previous;
	temp2=particle->next;
	BTX.ParticleNumber--;
	
	BTX.TankImpactCoeff[tankindex-1] += particle -> a;

	BTX.TimeofTankImpact[tankindex-1] = particle -> t;

	if(temp1==NULL&&temp2==NULL)
	{
		BTX.ParticleFirst=NULL;
		BTX.ParticleLast=NULL;
	}
	else if(temp2==NULL)
	{
		temp1->next=NULL;
		BTX.ParticleLast=temp1;
	}	
	else if(temp1==NULL)
	{
		temp2->previous=NULL;
		BTX.ParticleFirst=temp2;
	}
	else
	{
		temp1->next=temp2;
		temp2->previous=temp1;
	}
	particle->previous = BTX.FreeParticle;
	particle->next = NULL;
	BTX.FreeParticle = particle;
	return(0);
}

//splict partile at a junction based on flow rates at inflow pipes
int splitparticle(struct Sparticle * element, int upnode)
{	
	struct Sparticle* temp;
	float in_flow=0, element_a;
	int ninlink, i;
	int inlink[20];
	float flow[20];
	float distance[20];

	int maxinlink=20;/*assume maximum number of inflow link is 20*/
//	float inflow(int *, int *, float *, float *, int);/*routine to get inflow information*/
		
	ninlink = 0;
	element_a = element ->a;

	in_flow = inflow(&ninlink, inlink, flow, distance, upnode);

	if (ninlink == 0)
	{
		if (element->a <= 1.0)
		{
			eraseparticle(element);
			return -1;
		}
		else 
			return -1;
	}
	
	if (ninlink == 1&&in_flow == 0.0)
	{
		element->link = inlink[0];
		return 0;
	}

	/*update parent particle information*/
	element->a=flow[0]/in_flow*element_a;
	element->link=inlink[0];
	element->x=distance[0];
	/*generate new particle and add to the end of link list*/
	temp=BTX.ParticleLast;
	for(i=1;i<=ninlink-1;i++)
	{
		if (flow[i] != 0.0)
		{
			temp->next = generateparticle();
			if(temp->next == NULL)return(ERR_MEMORY); 
			temp->next->t = element->t;
			temp->next->a = flow[i]/in_flow*element_a;

			temp->next->link = inlink[i];
			temp->next->x = distance[i];
			temp->next->previous = temp;
			temp = temp->next;
		}
	}
	temp->next = NULL;
	BTX.ParticleLast = temp;

	BTX.ParticleNumber += ninlink - 1;
	return(0);
}


#include "epanetbtx.h"
#include "btxtypes.h"
#include "btxfuns.h"
#include "epanet2.h"
#include "mempool.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

extern BTXproject BTX;
//generate node neighborhood information
int getADJ()
{

	Padj temp1, temp2;
	int linkindex, fromnode, tonode, nnodes, nlinks;


	nnodes = BTX.Network.nnodes;
	nlinks = BTX.Network.nlinks;

	BTX.ADJ=(Padj *)calloc(nnodes, sizeof(Padj));
	if(BTX.ADJ==NULL) return (ERR_MEMORY);
  

	/*For each link*/
	for(linkindex=1;linkindex<=nlinks;linkindex++)
	{	
		ENgetlinknodes(linkindex, &fromnode, &tonode);
		temp1 = BTX.ADJ[fromnode-1];		
		if(temp1==NULL)
		{
			temp1=(Padj)calloc(1,sizeof(struct Sadj));
			if(temp1==NULL) return(ERR_MEMORY);
      temp1->next = NULL;
			BTX.ADJ[fromnode-1] = temp1;
		}
		else
		{
			while(temp1->next!=NULL)
				temp1=temp1->next;
			temp1->next=(struct Sadj *)calloc(1,sizeof(struct Sadj));
			if(temp1->next==NULL) return(ERR_MEMORY);
			temp1=temp1->next;
		}

		temp1->next=NULL;	
		temp1->flag=1;//indicating from node
		temp1->nodeindex=tonode;
		temp1->linkindex=linkindex;
 	
		temp2 = BTX.ADJ[tonode-1];
		if(temp2==NULL)
		{	
			temp2=(struct Sadj *)calloc(1,sizeof(struct Sadj));		
			if(temp2==NULL)return(ERR_MEMORY);
      temp2->next = NULL;
			BTX.ADJ[tonode-1] = temp2;
		}
		else
		{
			while(temp2->next!=NULL)temp2=temp2->next;
			temp2->next=(struct Sadj *)calloc(1,sizeof(struct Sadj));
			if(temp2->next==NULL) return(ERR_MEMORY);
			temp2=temp2->next;
		}
	
		temp2->next=NULL;
		temp2->flag=-1;//indicating tonode	
		temp2->nodeindex=fromnode;
		temp2->linkindex=linkindex;
	}
	return(0);
}

//read pipe flow rate
float readflowrate(int which_link, int which_step)
{	
	float flowrate;
	flowrate = BTX.LinkFlow[which_step-1][which_link-1];
	return(flowrate);
}

int prepareBT(int outnodeindex, float outtime)
{
	int i, nlinks = BTX.Network.nlinks, ntanks = BTX.Network.ntanks, meettank, position;
	int errcode = 0;
	float flowrate, tankinflow, tankoutflow;
	int ninlink;
	int inlink[20];
	float flow[20];
	float distance[20];

	BTX.Outtime = outtime*3600.0;
	freepath();
	errcode = initparticle(outnodeindex, outtime);
	if (errcode) return (errcode);
	BTX.Time = outtime*3600.0;
	BTX.PathNumber = 0;

	BTX.Nstep = 1;
	while(BTX.SaveTime[BTX.Nstep] < outtime*3600)
		BTX.Nstep++;

	for ( i = 0; i < nlinks; i++)
		BTX.PathFlag[i] = 0;
	
	BTX.TotalErased = 0.0;
	for(i = 0; i < ntanks; i++)
	{
		BTX.TimeofTankImpact[i] = BTX.Time;
		BTX.TankImpactCoeff[i] = 0;
		BTX.TankMainImpactCoeff[i] = 0;
	}
    meettank = 0;
	
	if (BTX.Node[outnodeindex].inputflag == 1 && outnodeindex <= (BTX.Network.nnodes-BTX.Network.ntanks))
	{
		if (BTX.Node[outnodeindex].sourcetype == EN_CONCEN)	/*concentration source*/
		{
			position = ((int)(BTX.ParticleFirst->t/(3600*BTX.Network.impactstep)));
			BTX.Node[outnodeindex].impact[position] += BTX.ParticleFirst->a/1000.0;
		}

		else if (BTX.Node[outnodeindex].sourcetype == EN_SETPOINT)	/*setpoint source*/
		{
			position = ((int)(BTX.ParticleFirst->t/(3600*BTX.Network.impactstep)));
			BTX.Node[outnodeindex].impact[position] += BTX.ParticleFirst->a/1000.0;
			eraseparticle(BTX.ParticleFirst);
			return (errcode);
		}
		else if (BTX.Node[outnodeindex].sourcetype == EN_FLOWPACED)
		{
			position = ((int)(BTX.ParticleFirst->t/(3600*BTX.Network.impactstep)));
			BTX.Node[outnodeindex].impact[position] += BTX.ParticleFirst->a/1000.0;
		}
		else if (BTX.Node[outnodeindex].sourcetype == EN_MASS)
		{
			flowrate = flowofmasssource(outnodeindex); /* L/minute */
			position = ((int)(BTX.ParticleFirst->t/(3600*BTX.Network.impactstep)));
			if (flowrate > 1.0)
				BTX.Node[outnodeindex].impact[position] += BTX.ParticleFirst->a/(flowrate*1000.0);
		}
	}		


    /*If output node is a tank*/
	for(i = 0; i < ntanks; i++)
	{
		if(outnodeindex == BTX.Tank[i].nodeindex)
		{
			if (BTX.Tank[i].mixtype == MIXED || BTX.Tank[i].mixtype == MIX2)
			{
				particletotank(BTX.ParticleFirst, i+1);//delete particle in i+1 th tank
				meettank = 1;
			}
			else if (BTX.Tank[i].mixtype == LIFO)
			{
				BTX.ParticleFirst->x = 0;
				BTX.ParticleFirst->link = 0 - (i+1);
			//	meettank = 1;
			}
			else   //FIFO
			{
				tankinflow = inflow(&ninlink, inlink, flow, distance, BTX.Tank[i].nodeindex);
				tankoutflow = outflow(BTX.Tank[i].nodeindex);
				BTX.ParticleFirst->x = BTX.TankVolume[i][BTX.Nstep-1]+(tankinflow+tankoutflow)*(BTX.ParticleFirst->t - BTX.SaveTime[BTX.Nstep-1]);;
				BTX.ParticleFirst->link = 0 - (i+1);
				meettank = 1; 			
			}
		}
	}

	if(!meettank)
		splitparticle(BTX.ParticleFirst, outnodeindex);

	return (0);

}


int addpath(int nodeindex, float timedelay, float impactcoeff)
{
	struct Spath * newpath;
	if(BTX.LastPath == NULL)
	{
		BTX.LastPath = (struct Spath *) calloc(1,sizeof(struct Spath));
		if (BTX.LastPath == NULL) return (ERR_MEMORY);
		BTX.LastPath->next = NULL;
		BTX.FirstPath = BTX.LastPath;
	}
	else
	{
		newpath = (struct Spath *) calloc(1,sizeof(struct Spath));
		if (newpath == NULL) return (ERR_MEMORY);
		newpath->next = NULL;
		BTX.LastPath->next = newpath;
		BTX.LastPath = newpath;
	}

	BTX.PathNumber ++; 
	BTX.LastPath->timedelay = timedelay;
	BTX.LastPath->endnodeindex = nodeindex;
    BTX.LastPath->impactcoefficient = impactcoeff;
	return 0;

}

int freepath()
{

	struct Spath *temp1, *temp2;
	temp1 = BTX.FirstPath;
	while(temp1!=NULL)
	{	
		temp2=temp1->next;
		temp1->next = NULL;
		free(temp1);
		temp1 = NULL;
		temp1 = temp2;
	}
	BTX.FirstPath = NULL;
	BTX.LastPath = NULL;
	BTX.PathNumber = 0;

	return (0);
}


int initparticle(int outnodeindex, float outtime)
{
	int linkindex;
	float length, distance;
	linkindex = BTX.ADJ[outnodeindex-1]->linkindex;
	ENgetlinkvalue(linkindex, EN_LENGTH, &length);
	
	if(BTX.ADJ[outnodeindex-1]->flag==1)
		distance = 0;
	else
		distance = length;
	
	/*generate the original backtrack particle at the output*/
	BTX.ParticleFirst = generateparticle();
	BTX.ParticleLast = BTX.ParticleFirst;
	
	BTX.ParticleFirst->link = linkindex;
	BTX.ParticleFirst->x = distance;
	BTX.ParticleFirst->t = outtime*3600;
	BTX.ParticleFirst->a = 1000;
	BTX.ParticleFirst->previous = NULL;
	BTX.ParticleFirst->next = NULL;
	BTX.ParticleNumber = 1;
	return (0);
}





float inflow(int * ninlink, int * inlink, float * flow, float * distance, int upnode)
/*
-----------------------------------------------------------------------------------
   Input:   upnode = node index              
   Output:  ninlink = number of links with water flowing to upnode
            inlink  = link index vector of inflow links
			flow    = flow rate of the inflow links
			distance = position (start or end) of the inflow link
   Returns: sum of inflow rates                             
   Purpose: get information about links with water flowing toward a node (upnode)
------------------------------------------------------------------------------------
*/
{
	struct Sadj * tempadj;
	int nodeindex, linkindex, flag, i;
	float flowrate, length, inflow;
	i = 0;
	inflow = 0.0;
//	tempadj = *(ADJ+upnode-1);
	tempadj = BTX.ADJ[upnode-1];
	/*check adjacent nodes and links*/
	while(tempadj != NULL)
	{
		nodeindex = tempadj->nodeindex;
		linkindex = tempadj->linkindex;
		flag = tempadj->flag;		
		flowrate = readflowrate(linkindex, BTX.Nstep);
		if(flowrate >= 0.0 && flag == -1)/*inflow link, the P node is the tonode and flow is from fromnode to tonode*/
		{
			inlink[i] = linkindex;
			flow[i] = Absolute(flowrate);
			inflow = inflow+flow[i];
			ENgetlinkvalue(linkindex, EN_LENGTH, &length);
			distance[i] = length;
			i = i+1;
		}
		
		else if(flowrate <= 0.0 && flag == 1)/*inflow link, the P node is the from node and flow is from tonode to fromnode*/
		{
			inlink[i] = linkindex;
			flow[i] = Absolute(flowrate);
			inflow = inflow+flow[i];
			distance[i] = 0;
			i = i+1;
		}

		tempadj = tempadj->next; /*check the next adjacent node*/
	}

	* ninlink = i;
	return(inflow);
}


float outflow(int upnode)
/*
-----------------------------------------------------------------------------------
   Input:   upnode = node index              
   Output:  none
   Returns: sum of inflow rates                             
   Purpose: calculate the flow rate leaving a node
------------------------------------------------------------------------------------
*/
{
	struct Sadj * tempadj;
	int nodeindex, linkindex, flag, i;
	float flowrate, inflow, outflow;
	i = 0;
	inflow = 0.0;
	outflow = 0.0;
//	tempadj = *(ADJ+upnode-1);
	tempadj = BTX.ADJ[upnode-1];
	/*check adjacent nodes and links*/
	while(tempadj != NULL)
	{
		nodeindex = tempadj->nodeindex;
		linkindex = tempadj->linkindex;
		flag = tempadj->flag;		
		flowrate = readflowrate(linkindex, BTX.Nstep);
		if(flowrate >= 0.0 && flag == -1)
		{
			outflow = outflow;
		}
		else if(flowrate <=0.0 && flag == 1)/*inflow link, the P node is the from node and flow is from tonode to fromnode*/
		{
			outflow = outflow;
		}
		else
			outflow -= Absolute(flowrate);

		tempadj = tempadj->next; /*check the next adjacent node*/
	}

	return(outflow);
}


float netflow(int node)
/*
-----------------------------------------------------------------------------------
   Input:   upnode = node index              
   Output:  none
   Returns: sum of inflow and outflow rates                             
   Purpose: calculate the net flow rates flowing to a node
------------------------------------------------------------------------------------
*/
{

	struct Sadj * tempadj;
	int nodeindex, linkindex, flag, i;
	float flowrate, netflow;
	i = 0;
	netflow = 0.0;
	tempadj = BTX.ADJ[node-1];
	/*check adjacent nodes and links*/
	while(tempadj != NULL)
	{
		nodeindex = tempadj->nodeindex;
		linkindex = tempadj->linkindex;
		flag = tempadj->flag;		
		flowrate = readflowrate(linkindex, BTX.Nstep);
		if(flowrate >= 0.0 && flag == -1)
		{
			netflow += flowrate;
		}
		else if(flowrate <=0.0 && flag == 1)/*inflow link, the P node is the from node and flow is from tonode to fromnode*/
		{
			netflow += Absolute(flowrate);
		}
		else
			netflow -= Absolute(flowrate);

		tempadj = tempadj->next; /*check the next adjacent node*/
	}

	return(netflow);

}

 
int clearactivelist()
{
	struct Sparticle * temp;
	struct Sparticle * element;
	
	element = BTX.ParticleFirst;
	while(element != NULL)
	{
		temp = element->next;
		eraseparticle(element);
		element = temp;
	}

	return (0);
}

//generate a new particle
struct Sparticle * generateparticle()
{
	struct Sparticle * newparticle;

    if (BTX.FreeParticle != NULL)
    {
       newparticle = BTX.FreeParticle;
       BTX.FreeParticle = newparticle->previous;
    }
    else
    {
        newparticle = (struct Sparticle *) Alloc(sizeof(struct Sparticle));
    }

	if ( newparticle != NULL)
	{
		newparticle->previous = NULL;
		newparticle->next = NULL;
	}
	return (newparticle);
}

int readhydraulics()
{
	float flow;
	int  nnodes, nlinks, ntanks, errcode, nsteps, i, j;

	errcode = 0;
	nnodes = BTX.Network.nnodes;
	nlinks = BTX.Network.nlinks;
	ntanks = BTX.Network.ntanks;
	nsteps = BTX.Network.nsteps;

	fseek(BTX.HydFile, BTX.HydOffset, SEEK_SET);

	for(i = 0; i <= nsteps-1; i++)
	{
		fread(&BTX.SaveTime[i], sizeof(int),1,BTX.HydFile);
		fseek(BTX.HydFile, 2*nnodes*sizeof(float), SEEK_CUR);
		fread(BTX.LinkFlow[i], sizeof(float), nlinks, BTX.HydFile);
		fseek(BTX.HydFile, 2*nlinks*sizeof(float), SEEK_CUR);
		fread(&BTX.SaveStep[i], sizeof(int),1,BTX.HydFile);
	}

	/*calculating tankvolume*/
	for ( j = 1; j <= ntanks; j++)
	{
		BTX.TankVolume[j-1][0] = BTX.Tank[j-1].initvolume;
		for ( i = 2; i <= nsteps; i++)
		{
			BTX.Nstep = i-1;
			flow = netflow(BTX.Tank[j-1].nodeindex); 
			BTX.TankVolume[j-1][i-1] = BTX.TankVolume[j-1][i-2]+flow*BTX.SaveStep[i-2];
		}
	}


	BTX.HydrauInforFlag = 1;
	return (0);

}

int eraseparticle(struct Sparticle * particle)
{
	struct Sparticle *temp1, *temp2;
	
	/*remove particle from active list*/

	temp1 = particle->previous;
	temp2 = particle->next;
	BTX.ParticleNumber--;
	BTX.TotalErased += particle->a;
	if(temp1 == NULL&&temp2 == NULL)
	{
		BTX.ParticleFirst = NULL;
		BTX.ParticleLast = NULL;
	}
	else if(temp2==NULL)
	{
		temp1->next = NULL;
		BTX.ParticleLast = temp1;
	}	
	else if(temp1 == NULL)
	{
		temp2->previous = NULL;
		BTX.ParticleFirst = temp2;
	}
	else
	{
		temp1->next = temp2;
		temp2->previous = temp1;
	}
	particle->previous = BTX.FreeParticle;
	particle->next = NULL;
	BTX.FreeParticle = particle;
//	free(particle);
	return(0);
}

/*decay rate calculation*/
float getpiperate(int k, float velocity, float diameter, float length)
/*--------------------------------------------------------------*/
/* Input:   k = link index                                      */
/* Output:  returns overall reaction rate coeff. for link k     */
/* Purpose: finds combined bulk & wall reaction rate coeffs.    */
/*                                                              */
/*      Note: Laminar flow:   Re <= 2300                        */
/*--------------------------------------------------------------*/
{
   float kw, kb, y, Sh, kf, Re, Sc;
   
   ENgetlinkvalue(k, EN_KWALL, &kw);
   ENgetlinkvalue(k, EN_KBULK, &kb);
//   if (kb<0)kb=Absolute(kb);
   if (kw == 0.0) return(kb);
//   if (kw<0)kw=Absolute(kw);
   if (velocity<= TINY) return(kb);//kf = 0.0;
   else
   {
      Sc=(float)(Viscos/Diffus);
      Re =(float)(velocity*diameter/Viscos);               /* Reynolds No.      */
      if (Re >= 2300.0)                    /* Turbulent flow    */
         /*Sh = 0.023*pow(Re,0.83)*pow(Sc,0.333);*/
		 Sh =(float)(0.0149*pow(Re,0.88)*pow(Sc,0.333));
      else
      {                                    /* Laminar flow      */
         y = diameter/length*Re*Sc;
         Sh =(float)(3.65+0.0668*y/(1.0+0.04*pow(y,0.667)));
      }
      kf = (float)(24*3600*Sh*Diffus/diameter);                    /* Mass trans coeff,  fps */
      kw = (float)(4.0*kw*kf/(diameter*(kf+Absolute(kw))));  /* Wall coeff., 1/sec*/
   }
   return(kw+kb);
} /* End of piperate */

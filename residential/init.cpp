/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file init.cpp
	@addtogroup residential Residential loads (residential)
	@ingroup modules
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "residential.h"

// obsolete as of 3.0: #include "house_a.h"
#include "appliance.h"
#include "clotheswasher.h"
#include "dishwasher.h"
#include "lights.h"
#include "microwave.h"
#include "occupantload.h"
#include "plugload.h"
#include "range.h"
#include "refrigerator.h"
#include "waterheater.h"
#include "freezer.h"
#include "dryer.h"
#include "evcharger.h"
#include "zipload.h"
#include "thermal_storage.h"
#include "evcharger_det.h"

#include "residential_enduse.h"
#include "house_e.h"

complex default_line_voltage[3] = {complex(240,0,A),complex(120,0,A),complex(120,0,A)};
complex default_line_current[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
complex default_line_shunt[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
complex default_line_power[3] = {complex(0,0,J),complex(0,0,J),complex(0,0,J)};
int default_meter_status = 1;	//In service
bool ANSI_voltage_check = true;	//Flag to enable/disable ANSI voltage violation checks
double default_outdoor_temperature = 74.0;
double default_humidity = 75.0;
double default_solar[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
int64 default_etp_iterations = 100;

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	gl_global_create("residential::default_line_voltage",PT_complex,&default_line_voltage,PT_SIZE,3,PT_UNITS,"V",PT_DESCRIPTION,"line voltage to use when no circuit is attached",NULL);
	gl_global_create("residential::default_line_current",PT_complex,&default_line_current,PT_SIZE,3,PT_UNITS,"A",PT_DESCRIPTION,"line current calculated when no circuit is attached",NULL);
	gl_global_create("residential::default_outdoor_temperature",PT_double,&default_outdoor_temperature,PT_UNITS,"degF",PT_DESCRIPTION,"outdoor air temperature when no climate data is found",NULL);
	gl_global_create("residential::default_humidity",PT_double,&default_humidity,PT_UNITS,"%",PT_DESCRIPTION,"humidity when no climate data is found",NULL);
	gl_global_create("residential::default_solar",PT_double,&default_solar,PT_SIZE,9,PT_UNITS,"Btu/sf",PT_DESCRIPTION,"solar gains when no climate data is found",NULL);
	gl_global_create("residential::default_etp_iterations",PT_int64,&default_etp_iterations,PT_DESCRIPTION,"number of iterations ETP solver will run",NULL);
	gl_global_create("residential::ANSI_voltage_check",PT_bool,&ANSI_voltage_check,PT_DESCRIPTION,"enable or disable messages about ANSI voltage limit violations in the house",NULL);
	gl_global_create("residential::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the residential module",NULL);
	gl_global_create("residential::deltamode_timestep", PT_double, &deltamode_timestep_publish,PT_UNITS,"ns",PT_DESCRIPTION,"Desired minimum timestep for deltamode-related simulations",NULL);


	new residential_enduse(module);
	new appliance(module);
	// obsolete as of 3.0: new house(module);
	new house_e(module);
	new waterheater(module);
	new lights(module);
	new refrigerator(module);
	new clotheswasher(module);
	new dishwasher(module);
	new occupantload(module);
	new plugload(module);
	new microwave(module);
	new range(module);
	new freezer(module);
	new dryer(module);
	new evcharger(module);
	new ZIPload(module);
	new thermal_storage(module);
	new evcharger_det(module);

	/* always return the first class registered */
	return residential_enduse::oclass;
}

//Call function for scheduling deltamode
void schedule_deltamode_start(TIMESTAMP tstart)
{
	if (enable_subsecond_models == true)	//Make sure the overall mode is enabled
	{
		if ( (tstart<deltamode_starttime) && ((tstart-gl_globalclock)<0x7fffffff )) // cannot exceed 31 bit integer
			deltamode_starttime = tstart;	//Smaller and valid, store it
	}
	else
	{
		GL_THROW("residential: a call was made to deltamode functions, but subsecond models are not enabled!");
	}
}

// add export functions to support deltamode

//deltamode_desired function
//Module-level call to determine when the next object expects
//to enter deltamode, even if it is now.
//Returns time to next deltamode in integers seconds.
//i.e., 0 = deltamode now, 1 = deltamode 1 second from the current simulation time
//Return DT_INFINITY for no deltamode desired
EXPORT unsigned long deltamode_desired(int *flags)
{
	unsigned long dt_val;

	if (enable_subsecond_models == true)	//Make sure we even want to run deltamode
	{
		//See if the value is worth storing, or irrelevant at this time
		// ?? how is deltamode_starttime decided??
		if ((deltamode_starttime>=gl_globalclock) && (deltamode_starttime<TS_NEVER))
		{
			//Set the flag to do a soft deltamode transition
			*flags |= DMF_SOFTEVENT;

			//Compute the difference to get the incremental time needed
			dt_val = (unsigned long)(deltamode_starttime - gl_globalclock);

			gl_debug("residential: deltamode desired in %d", dt_val);
			return dt_val;
		}
		else
		{
			//No scheduled deltamode, or it wasn't a valid value
			return DT_INFINITY;
		}
	}
	else
	{
		return DT_INFINITY;
	}
}


//preudpate function of deltamode
//Module-level call at beginning of each deltamode simulation
//Returns the timestep this module requires - to be used to determine minimimum
//detamode simulation stepsize
EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	if (enable_subsecond_models == true)
	{
		if (deltamode_timestep_publish<=0.0)
		{
			gl_error("residential::deltamode_timestep must be a positive, non-zero number!");
			/*  TROUBLESHOOT
			The value for deltamode_timestep, as specified as the module level in powerflow, must be a positive, non-zero number.
			Please use such a number and try again.
			*/

			return DT_INVALID;
		}
		else
		{
			//Cast in the published value
			// ?? why +0.5 ??
			deltamode_timestep = (unsigned long)(deltamode_timestep_publish+0.5);

			//Return it
			return deltamode_timestep;
		}
	}
	else	//Not desired, just return an arbitrarily large value
	{
		return DT_INFINITY;
	}
}


typedef bool (*SUBSECONDCALL)(unsigned int64,unsigned long);

typedef struct s_subsecond_call {
	SUBSECONDCALL call;
	OBJECT *obj;
	struct s_subsecond_call *next;
} SUBSECONDCALLLIST;

static SUBSECONDCALLLIST *calllist = NULL;

void add_subsecond_call(SUBSECONDCALL *call, OBJECT *obj)
{
	SUBSECONDCALLLIST *item = (SUBSECONDCALLLIST*)malloc(sizeof(SUBSECONDCALLLIST));
	item->call = call;
	item->obj = obj;
	item->next = calllist;
	callist = item;
}

//interupdate function of deltamode
//Module-level call for each timestep of deltamode
//Ideally, all deltamode objects coordinate through their module call, not their individual "update" call
//Returns SIMULATIONMODE - SM_DELTA, SM_DELTA_ITER, SM_EVENT, or SM_ERROR
EXPORT SIMULATIONMODE interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	SIMULATIONMODE function_status = SM_EVENT;
	for ( SUBSECONDCALL *item = calllist ; item != NULL ; item=item->next )
	{
		if ( item->call(item->obj,delta_time,dt) )
			function_status = SM_DELTA;
	}
	return function_status;
}

//postupdate function of deltamode
//Executes after all objects in the simulation agree to go back to event-driven mode
//Return value is a SUCCESS/FAILURE
EXPORT STATUS postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	if (enable_subsecond_models == true)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;

		//Deflag the timestep variable as well
		deltatimestep_running = -1.0;
		
		return SUCCESS;
	}
	else	//Deltamode not wanted, just "succeed"
	{
		return SUCCESS;
	}
}



CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	return 0;
}


/**@}**/

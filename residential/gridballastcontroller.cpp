/*
 * gridballastcontroller.cpp
 *
 *  Created on: May 8, 2017
 *      Author: jingkungao
 *  The GridBallast Controller to return ON/OFF status bases on frequency
 *  the controller will only affect the m_t, the idea is that,
 *  given the frequency information,
 *  decide whether to bring up the load or shut down the load.
 *  the controller can also be called directly to override the thermostat_controller
 *  by letting enable_lock to be True and providing values for lock_STATUS
 */

#include "gridballastcontroller.h"

#define TSTAT_PRECISION 0.01

namespace gridballastcontroller {

gridballastcontroller::gridballastcontroller() {
	// set default values for the private variables
	freq_lowlimit = 59.9;
	freq_uplimit = 60.1;
	T_setpoint = 120;
	T_deadband = 2;
}

//overload constructor
gridballastcontroller::gridballastcontroller(double freq_low,
		double freq_up,double T_sp,double T_db) {
	// set private variables with chosen values during constructions
	freq_lowlimit = freq_low;
	freq_uplimit = freq_up;
	T_setpoint = T_sp;
	T_deadband = T_db;
}

void gridballastcontroller::set_parameters(double freq_low,
		double freq_up,double T_sp,double T_db) {
	// methods which can be called to set private variables
	freq_lowlimit = freq_low;
	freq_uplimit = freq_up;
	T_setpoint = T_sp;
	T_deadband = T_db;
}

gridballastcontroller::~gridballastcontroller() {
	// TODO Auto-generated destructor stub
}

bool gridballastcontroller::check_freq_violation(double freq_t) {
	return (freq_t < freq_lowlimit) || (freq_t > freq_uplimit);
}

bool gridballastcontroller::check_thermal_violation(double T_t) {
	return ((T_t - TSTAT_PRECISION) < (T_setpoint - T_deadband/2)) || ((T_t  + TSTAT_PRECISION) > (T_setpoint + T_deadband/2));
}

/*
bool gridballastcontroller::lock_mode(bool circuit_status, bool force_ON, bool force_OFF){
	if (force_ON && force_OFF) {
		//std::cout << "Cannot be forced ON and OFF the same time!\n EXIT!" << std::endl;
		gl_error("Controller cannot be forced ON and OFF the same time once the lock is enabled!\n EXIT!" );
	}
	else if (!force_ON && !force_OFF) {
		//std::cout << "Enable lock but both force_ON and force_OFF are FALSE!\n EXIT!" << std::endl;
		gl_warning("Lock is enabled but both force_ON and force_OFF are FALSE!\n Using frequency control instead!");
	}
	else if (force_ON){
		return true;
	}
	else if (force_OFF){
		return false;
	}
	gl_warning("Incorrect input of lock mode, using original circuit status");
	return circuit_status;
}
*/

bool gridballastcontroller::frequency_controller(double freq_t, bool circuit_status,
		bool enable_lock, bool lock_STATUS){
	if (enable_lock) {
		circuit_status = lock_STATUS;
		return circuit_status;
	}
	// compare the frequency with lower and upper limit
	if (freq_t < freq_lowlimit){
		// turn OFF load if freq_t is too low
		circuit_status = false;
	}
	else if (freq_t > freq_uplimit){
		// turn ON the load if too high
		circuit_status = true;
	}
	// if none of the condition triggered, using the previous circuit_stauts
	return circuit_status;
}

bool gridballastcontroller::thermostat_controller(double T_t, bool circuit_status,
			bool reverse_ON_OFF, bool enable_freq_control, double  freq_t,
			bool enable_lock, bool lock_STATUS, bool lock_OVERRIDE_TS){
//	static bool first = true;
//	if (first){
//		gl_output("T_setpoint:%.2f\n T_deadband:%.2f\n enable_freq_control:%d\n freq_t:%.2f",
//				T_setpoint,T_deadband,enable_freq_control,freq_t);
//		first = false;
//	}

	// first check if we want to override the thermostat set point when the lock is enabled
	if (enable_lock && lock_OVERRIDE_TS) {
		return lock_STATUS;
	}

	// Otherwise, we put the the thermostat in the first priority, then we check
	// the frequency controller
	if ((T_t - TSTAT_PRECISION) < (T_setpoint - T_deadband/2)) {
		// turn on the load if reverse_ON_OFF false otherwise turn off
		circuit_status = !reverse_ON_OFF;
	}
	else if ((T_t  + TSTAT_PRECISION) > (T_setpoint + T_deadband/2)) {
		// turn off the load if reverse_ON_OFF false otheweise turn on
		circuit_status = reverse_ON_OFF;
	}
	else if (enable_freq_control) {
		/*
		 *  we are in the safe zone of making circuit_status true or false
		 *  without going out of the thermal band. We want to impose the
		 *  control functionality only when it won't compromise the users' comfort level
		 *  We don't consider the lock in this situation.
		 *  Call frequency_controller directly if people want to enable lock.
		 */
		circuit_status = frequency_controller(freq_t, circuit_status, enable_lock, lock_STATUS);
	}
	// if none of the conditions triggered, using the previous circuit_status
	return circuit_status;
}
} /* namespace gridballastcontroller */

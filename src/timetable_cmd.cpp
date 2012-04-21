/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file timetable_cmd.cpp Commands related to time tabling. */

#include "stdafx.h"
#include "command_func.h"
#include "company_func.h"
#include "date_func.h"
#include "window_func.h"
#include "vehicle_base.h"
#include "settings_type.h"
#include "cmd_helper.h"

#include "table/strings.h"

/**
 * Change/update a particular timetable entry.
 * @param v            The vehicle to change the timetable of.
 * @param order_number The index of the timetable in the order list.
 * @param val          The new data of the timetable entry.
 * @param mtf          Which part of the timetable entry to change.
 */
static void ChangeTimetable(Vehicle *v, VehicleOrderID order_number, uint16 val, ModifyTimetableFlags mtf)
{
	Order *order = v->GetOrder(order_number);
	int delta = 0;

	switch (mtf) {
		case MTF_WAIT_TIME:
			delta = val - order->wait_time;
			order->wait_time = val;
			break;

		case MTF_TRAVEL_TIME:
			delta = val - order->travel_time;
			order->travel_time = val;
			break;

		case MTF_TRAVEL_SPEED:
			order->max_speed = val;
			break;

		default:
			NOT_REACHED();
	}
	v->orders.list->UpdateOrderTimetable(delta);

	for (v = v->FirstShared(); v != NULL; v = v->NextShared()) {
		if (v->cur_real_order_index == order_number && v->current_order.Equals(*order)) {
			switch (mtf) {
				case MTF_WAIT_TIME:
					v->current_order.wait_time = val;
					break;

				case MTF_TRAVEL_TIME:
					v->current_order.travel_time = val;
					break;

				case MTF_TRAVEL_SPEED:
					v->current_order.max_speed = val;
					break;

				default:
					NOT_REACHED();
			}
		}
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}
}

/**
 * Change timetable data of an order.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Various bitstuffed elements
 * - p1 = (bit  0-19) - Vehicle with the orders to change.
 * - p1 = (bit 20-27) - Order index to modify.
 * - p1 = (bit 28-29) - Timetable data to change (@see ModifyTimetableFlags)
 * @param p2 The amount of time to wait.
 * - p2 = (bit  0-15) - The data to modify as specified by p1 bits 28-29.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdChangeTimetable(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	VehicleOrderID order_number = GB(p1, 20, 8);
	Order *order = v->GetOrder(order_number);
	if (order == NULL || order->IsType(OT_IMPLICIT)) return CMD_ERROR;

	ModifyTimetableFlags mtf = Extract<ModifyTimetableFlags, 28, 2>(p1);
	if (mtf >= MTF_END) return CMD_ERROR;

	int wait_time   = order->wait_time;
	int travel_time = order->travel_time;
	int max_speed   = order->max_speed;
	switch (mtf) {
		case MTF_WAIT_TIME:
			wait_time = GB(p2, 0, 16);
			break;

		case MTF_TRAVEL_TIME:
			travel_time = GB(p2, 0, 16);
			break;

		case MTF_TRAVEL_SPEED:
			max_speed = GB(p2, 0, 16);
			if (max_speed == 0) max_speed = UINT16_MAX; // Disable speed limit.
			break;

		default:
			NOT_REACHED();
	}

	if (wait_time != order->wait_time) {
		switch (order->GetType()) {
			case OT_GOTO_STATION:
				if (order->GetNonStopType() & ONSF_NO_STOP_AT_DESTINATION_STATION) return_cmd_error(STR_ERROR_TIMETABLE_NOT_STOPPING_HERE);
				break;

			case OT_CONDITIONAL:
				break;

			default: return_cmd_error(STR_ERROR_TIMETABLE_ONLY_WAIT_AT_STATIONS);
		}
	}

	if (travel_time != order->travel_time && order->IsType(OT_CONDITIONAL)) return CMD_ERROR;
	if (max_speed != order->max_speed && (order->IsType(OT_CONDITIONAL) || v->type == VEH_AIRCRAFT)) return CMD_ERROR;

	if (flags & DC_EXEC) {
		if (wait_time   != order->wait_time)   ChangeTimetable(v, order_number, wait_time,   MTF_WAIT_TIME);
		if (travel_time != order->travel_time) ChangeTimetable(v, order_number, travel_time, MTF_TRAVEL_TIME);
		if (max_speed   != order->max_speed)   ChangeTimetable(v, order_number, max_speed,   MTF_TRAVEL_SPEED);
	}

	return CommandCost();
}

/**
 * Clear the lateness counter to make the vehicle on time.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Various bitstuffed elements
 * - p1 = (bit  0-19) - Vehicle with the orders to change.
 * @param p2 unused
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdSetVehicleOnTime(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		v->lateness_counter = 0;
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}

	return CommandCost();
}

/**
 * Set the start date of the timetable.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Vehicle id.
 * @param p2 The timetable start date.
 * @param text Not used.
 * @return The error or cost of the operation.
 */
CommandCost CmdSetTimetableStart(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	Vehicle *v = Vehicle::GetIfValid(GB(p1, 0, 20));
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	/* Don't let a timetable start more than 15 years into the future or 1 year in the past. */
	Date start_date = (Date)p2;
	if (start_date < 0 || start_date > MAX_DAY) return CMD_ERROR;
	if (start_date - _date > 15 * DAYS_IN_LEAP_YEAR) return CMD_ERROR;
	if (_date - start_date > DAYS_IN_LEAP_YEAR) return CMD_ERROR;

	if (flags & DC_EXEC) {
		v->lateness_counter = 0;
		ClrBit(v->vehicle_flags, VF_TIMETABLE_STARTED);
		v->timetable_start = start_date;

		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}

	return CommandCost();
}


/**
 * Start or stop filling the timetable automatically from the time the vehicle
 * actually takes to complete it. When starting to autofill the current times
 * are cleared and the timetable will start again from scratch.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Vehicle index.
 * @param p2 Various bitstuffed elements
 * - p2 = (bit 0) - Set to 1 to enable, 0 to disable autofill.
 * - p2 = (bit 1) - Set to 1 to preserve waiting times in non-destructive mode
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdAutofillTimetable(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	VehicleID veh = GB(p1, 0, 20);

	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		if (HasBit(p2, 0)) {
			/* Start autofilling the timetable, which clears the
			 * "timetable has started" bit. Times are not cleared anymore, but are
			 * overwritten when the order is reached now. */
			SetBit(v->vehicle_flags, VF_AUTOFILL_TIMETABLE);
			ClrBit(v->vehicle_flags, VF_TIMETABLE_STARTED);

			/* Overwrite waiting times only if they got longer */
			if (HasBit(p2, 1)) SetBit(v->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME);

			v->timetable_start = 0;
			v->lateness_counter = 0;
		} else {
			ClrBit(v->vehicle_flags, VF_AUTOFILL_TIMETABLE);
			ClrBit(v->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME);
		}

		for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
			if (v2 != v) {
				/* Stop autofilling; only one vehicle at a time can perform autofill */
				ClrBit(v2->vehicle_flags, VF_AUTOFILL_TIMETABLE);
				ClrBit(v2->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME);
			}
			SetWindowDirty(WC_VEHICLE_TIMETABLE, v2->index);
		}
	}

	return CommandCost();
}

/**
 * Start or stop automatic management of timetables.
 * @param tile Not used.
 * @param flags Operation to perform.
 * @param p1 Vehicle index.
 * @param p2 Various bitstuffed elements
 * - p2 = (bit 0) - Set to 1 to enable, 0 to disable automation.
 * - p2 = (bit 1) - Ctrl was pressed. Used when disabling to keep times.
 * @param text unused
 * @return the cost of this operation or an error
 */

CommandCost CmdAutomateTimetable(TileIndex index, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (!_settings_game.order.timetable_automated) return CMD_ERROR;

	VehicleID veh = GB(p1, 0, 16);
	
	Vehicle *v = Vehicle::GetIfValid(veh);
	if (v == NULL || !v->IsPrimaryVehicle()) return CMD_ERROR;

	CommandCost ret = CheckOwnership(v->owner);
	if (ret.Failed()) return ret;

	if (flags & DC_EXEC) {
		for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
			if (HasBit(p2, 0)) {
				/* Automated timetable. Set flags and clear current times. */
				SetBit(v2->vehicle_flags, VF_AUTOMATE_TIMETABLE);
				ClrBit(v2->vehicle_flags, VF_AUTOFILL_TIMETABLE);
				ClrBit(v2->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME);
				ClrBit(v2->vehicle_flags, VF_TIMETABLE_STARTED);
				v2->timetable_start = 0;
				v2->lateness_counter = 0;
				v2->current_loading_time = 0;
				v2->ClearSeparation();
			} else {
				/* De-automate timetable. Clear flags. */
				ClrBit(v2->vehicle_flags, VF_AUTOMATE_TIMETABLE);
				ClrBit(v2->vehicle_flags, VF_AUTOFILL_TIMETABLE);
				ClrBit(v2->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME);
				v2->ClearSeparation();
				if (!HasBit(p2, 1)) {
					/* Ctrl wasn't pressed, so clear all timetabled times. */
					SetBit(v2->vehicle_flags, VF_TIMETABLE_STARTED);
					v2->timetable_start = 0;
					v2->lateness_counter = 0;
					v2->current_loading_time = 0;
					OrderList *orders = v2->orders.list;
					if (orders != NULL) {
						for (int i = 0; i < orders->GetNumOrders(); i++) {
							ChangeTimetable(v2, i, 0, MTF_WAIT_TIME);
							ChangeTimetable(v2, i, 0, MTF_TRAVEL_TIME);
						}
					}
				}
			}
			SetWindowDirty(WC_VEHICLE_TIMETABLE, v2->index);
		}
	}

	return CommandCost();
}

int TimeToFinishOrder(Vehicle *v, int n)
{
	int left;
	Order *order = v->GetOrder(n);
	assert(order != NULL);
	if ((v->cur_real_order_index == n) && (v->last_station_visited == order->GetDestination())) {
		if (order->wait_time == 0) return -1;
		if (v->current_loading_time > 0) {
			left = order->wait_time - v->current_order_time;
		} else {
			left = order->wait_time;
		}
		if (left < 0) left = 0;
	} else {
		left = order->travel_time;
		if (v->cur_real_order_index == n) left -= v->current_order_time;
		if (order->travel_time == 0 || order->wait_time == 0) return -1;
		if (left < 0) left = 0;
		left += order->wait_time;
	}
	return left;
}

int SeparationBetween(Vehicle *v1, Vehicle *v2)
{
	if (v1 == v2) return -1;
	int separation = 0;
	int time;
	int n = v1->cur_real_order_index;
	while (n != v2->cur_real_order_index) {
		time = TimeToFinishOrder(v1, n);
		if (time == -1) return -1;
		separation += time;
		n++;
		if (n >= v1->GetNumOrders()) n = 0;
	}
	int time1 = TimeToFinishOrder(v1, n);
	int time2 = TimeToFinishOrder(v2, n);
	if (time1 == -1 || time2 == -1) return -1;
	time = time1 - time2;
	if (time < 0) {
		for (n = 0; n < v1->GetNumOrders(); n++) {
			Order *order = v1->GetOrder(n);
			if (order->travel_time == 0 || order->wait_time == 0) return -1;
			time += order->travel_time + order->wait_time;
		}
	}
	separation += time;
	assert(separation >= 0);
	if (separation == 0) return -1;
	return separation;
}

void UpdateSeparationOrder(Vehicle *v_start)
{
	/* First check if we have a vehicle ahead, and if not search for one. */
	if (v_start->AheadSeparation() == NULL) {
		v_start->InitSeparation();
	}
	if (v_start->AheadSeparation() == NULL) {
		return;
	}
	/* Switch positions if necessary. */
	int swaps = 0;
	bool done = false;
	while (!done) {
		done = true;
		int min_sep = SeparationBetween(v_start, v_start->AheadSeparation());
		Vehicle *v = v_start;
		do {
			if (v != v_start) {
				int tmp_sep = SeparationBetween(v_start, v);
				if (tmp_sep < min_sep && tmp_sep != -1) {
					swaps++;
					if (swaps >= 50) {
						return;
					}
					done = false;
					v_start->ClearSeparation();
					v_start->AddToSeparationBehind(v);
					break;
				}
			}
			int separation_ahead = SeparationBetween(v, v->AheadSeparation());
			int separation_behind = SeparationBetween(v->BehindSeparation(), v);
			v->lateness_counter = (separation_ahead - separation_behind) / 2;
			if (separation_ahead == -1 || separation_behind == -1) v->lateness_counter = 0;
			v = v->AheadSeparation();
		} while (v != v_start);
	}
}



/**
 * Update the timetable for the vehicle.
 * @param v The vehicle to update the timetable for.
 * @param travelling Whether we just travelled or waited at a station.
 */
void UpdateVehicleTimetable(Vehicle *v, bool travelling)
{
	uint timetabled = travelling ? v->current_order.travel_time : v->current_order.wait_time;
	if (!travelling) v->current_loading_time++; // +1 because this time is one tick behind
	uint time_taken = v->current_order_time;
	uint time_loading = v->current_loading_time;

	v->current_order_time = 0;
	v->current_loading_time = 0;

	if (v->current_order.IsType(OT_IMPLICIT)) return; // no timetabling of auto orders

	VehicleOrderID first_manual_order = 0;
	for (Order *o = v->GetFirstOrder(); o != NULL && o->IsType(OT_IMPLICIT); o = o->next) {
		++first_manual_order;
	}

	bool just_started = false;

	/* Start automated timetables at first opportunity */
	if (!HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED) && HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE)) {
		if (_settings_game.order.timetable_separation) v->ClearSeparation();
		SetBit(v->vehicle_flags, VF_TIMETABLE_STARTED);
		v->lateness_counter = 0;
		if (_settings_game.order.timetable_separation) UpdateSeparationOrder(v);
		for (v = v->FirstShared(); v != NULL; v = v->NextShared()) {
			SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
		}
		return;
	}

	/* This vehicle is arriving at the first destination in the timetable. */
	if (v->cur_real_order_index == first_manual_order && travelling) {
		/* If the start date hasn't been set, or it was set automatically when
		 * the vehicle last arrived at the first destination, update it to the
		 * current time. Otherwise set the late counter appropriately to when
		 * the vehicle should have arrived. */
		just_started = !HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED);

		if (v->timetable_start != 0) {
			v->lateness_counter = (_date - v->timetable_start) * DAY_TICKS + _date_fract;
			v->timetable_start = 0;
		}

		SetBit(v->vehicle_flags, VF_TIMETABLE_STARTED);
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}

	if (!HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED)) return;

	if (HasBit(v->vehicle_flags, VF_AUTOFILL_TIMETABLE)) {
		if (travelling && !HasBit(v->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME)) {
			/* Need to clear that now as otherwise we are not able to reduce the wait time */
			v->current_order.wait_time = 0;
		}

		if (just_started) return;

		/* Modify station waiting time only if our new value is larger (this is
		 * always the case when we cleared the timetable). */
		if (!v->current_order.IsType(OT_CONDITIONAL) && (travelling || time_taken > v->current_order.wait_time)) {
			/* Round the time taken up to the nearest day, as this will avoid
			 * confusion for people who are timetabling in days, and can be
			 * adjusted later by people who aren't.
			 * For trains/aircraft multiple movement cycles are done in one
			 * tick. This makes it possible to leave the station and process
			 * e.g. a depot order in the same tick, causing it to not fill
			 * the timetable entry like is done for road vehicles/ships.
			 * Thus always make sure at least one tick is used between the
			 * processing of different orders when filling the timetable. */
			time_taken = CeilDiv(max(time_taken, 1U), DAY_TICKS) * DAY_TICKS;

			ChangeTimetable(v, v->cur_real_order_index, time_taken, travelling ? MTF_TRAVEL_TIME : MTF_WAIT_TIME);
		}

		if (v->cur_real_order_index == first_manual_order && travelling) {
			/* If we just started we would have returned earlier and have not reached
			 * this code. So obviously, we have completed our round: So turn autofill
			 * off again. */
			ClrBit(v->vehicle_flags, VF_AUTOFILL_TIMETABLE);
			ClrBit(v->vehicle_flags, VF_AUTOFILL_PRES_WAIT_TIME);
		}
		return;
	}

	if (just_started) return;

	/* Update the timetable to gradually shift order times towards the actual travel times. */
	if (timetabled != 0 && HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE)) {
		int32 new_time;
		if (travelling) {
			new_time = time_taken;
		} else {
			new_time = time_loading;
		}

		/* Check for too large a difference from expected time, and if so don't average. */
		if (!(new_time > (int32)timetabled * 2 || new_time < (int32)timetabled / 2)) {
			int arrival_error = timetabled - new_time;
			/* Compute running average, with sign conversion to avoid negative overflow. */
			new_time = ((int32)timetabled * 4 + new_time + 2) / 5;
			/* Use arrival_error to finetune order ticks. */
			if (arrival_error < 0) new_time++;
			if (arrival_error > 0) new_time--;
		} else if (new_time > (int32)timetabled * 10 && travelling) {
			/* Possible jam, clear time and restart timetable for all vehicles.
			 * Otherwise we risk trains blocking 1-lane stations for long times. */
			ChangeTimetable(v, v->cur_real_order_index, 0, travelling ? MTF_TRAVEL_TIME : MTF_WAIT_TIME);
			for (Vehicle *v2 = v->FirstShared(); v2 != NULL; v2 = v2->NextShared()) {
				if (_settings_game.order.timetable_separation) v2->ClearSeparation();
				ClrBit(v2->vehicle_flags, VF_TIMETABLE_STARTED);
				SetWindowDirty(WC_VEHICLE_TIMETABLE, v2->index);
			}
			return;
		}
		
		if (new_time < 1) new_time = 1;
		if (new_time != (int32)timetabled)
			ChangeTimetable(v, v->cur_real_order_index, new_time, travelling ? MTF_TRAVEL_TIME : MTF_WAIT_TIME);
	} else if (timetabled == 0 && HasBit(v->vehicle_flags, VF_AUTOMATE_TIMETABLE)) {
		/* Add times for orders that are not yet timetabled, even while not autofilling */
		if (travelling)
			ChangeTimetable(v, v->cur_real_order_index, time_taken, travelling ? MTF_TRAVEL_TIME : MTF_WAIT_TIME);
		else
			ChangeTimetable(v, v->cur_real_order_index, time_loading, travelling ? MTF_TRAVEL_TIME : MTF_WAIT_TIME);
	}

	/* Vehicles will wait at stations if they arrive early even if they are not
	 * timetabled to wait there, so make sure the lateness counter is updated
	 * when this happens. */
	if (timetabled == 0 && (travelling || v->lateness_counter >= 0)) return;

	if (_settings_game.order.timetable_separation && HasBit(v->vehicle_flags, VF_TIMETABLE_STARTED)) {
		v->current_order_time = time_taken;
		v->current_loading_time = time_loading;
		UpdateSeparationOrder(v);
		v->current_order_time = 0;
		v->current_loading_time = 0;
	} else {
		v->lateness_counter -= (timetabled - time_taken);
	}

	/* When we are more late than this timetabled bit takes we (somewhat expensively)
	 * check how many ticks the (fully filled) timetable has. If a timetable cycle is
	 * shorter than the amount of ticks we are late we reduce the lateness by the
	 * length of a full cycle till lateness is less than the length of a timetable
	 * cycle. When the timetable isn't fully filled the cycle will be INVALID_TICKS. */
	if (v->lateness_counter > (int)timetabled) {
		Ticks cycle = v->orders.list->GetTimetableTotalDuration();
		if (cycle != INVALID_TICKS && v->lateness_counter > cycle) {
			v->lateness_counter %= cycle;
		}
	}

	for (v = v->FirstShared(); v != NULL; v = v->NextShared()) {
		SetWindowDirty(WC_VEHICLE_TIMETABLE, v->index);
	}
}

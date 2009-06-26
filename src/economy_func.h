/* $Id$ */

/** @file economy_func.h Functions related to the economy. */

#ifndef ECONOMY_FUNC_H
#define ECONOMY_FUNC_H

#include "core/geometry_type.hpp"
#include "economy_type.h"
#include "cargo_type.h"
#include "vehicle_type.h"
#include "tile_type.h"
#include "town_type.h"
#include "industry_type.h"
#include "company_type.h"
#include "station_type.h"
#include "cargopacket.h"
#include <map>

void ResetPriceBaseMultipliers();
void SetPriceBaseMultiplier(uint price, byte factor);
void ResetEconomy();

extern const ScoreInfo _score_info[];
extern int _score_part[MAX_COMPANIES][SCORE_END];
extern Economy _economy;
/* Prices and also the fractional part. */
extern Prices _price;
extern uint16 _price_frac[NUM_PRICES];
extern Money  _cargo_payment_rates[NUM_CARGO];
extern uint16 _cargo_payment_rates_frac[NUM_CARGO];
typedef std::map<CargoID, CargoList::List> CargoReservation;

int UpdateCompanyRatingAndValue(Company *c, bool update);
void StartupIndustryDailyChanges(bool init_counter);

Money GetTransportedGoodsIncome(uint num_pieces, uint dist, byte transit_days, CargoID cargo_type);
uint MoveGoodsToStation(TileIndex tile, int w, int h, CargoID type, uint amount);

void PrepareUnload(Station * curr_station, Vehicle *front_v, StationID next_station_id);
void LoadUnloadStation(Station *st);

Money GetPriceByIndex(uint8 index);

#endif /* ECONOMY_FUNC_H */

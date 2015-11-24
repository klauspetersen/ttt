/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2014 Bert Vermeulen <bert@biot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "libsigrok.h"
#include "libsigrok-internal.h"

/** @cond PRIVATE */

#ifdef HAVE_HW_SALEAE_LOGIC16
extern SR_PRIV struct sr_dev_driver saleae_logic16_driver_info;
#endif

/** @endcond */

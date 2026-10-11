// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#pragma once

#include "types.h"

namespace dollfin {

class Store;

// How stock follows logged doses (D-033). A taken dose takes one off when stock
// is counted in doses, otherwise the schedule's units per dose (D-031). What was
// actually taken off is recorded on the intake so undo gives back exactly that.
class StockLedger
{
public:
    // Amount one dose of this regimen takes off this stock.
    static double perDose(const Stock &stock, const Regimen &regimen);

    // Take one dose off the medication's stock. Returns what was actually taken
    // off: 0 when stock is not tracked, less than a dose when it runs out.
    static double consume(Store &store, qint64 medicationId, const Regimen &regimen);

    // Give back what a taken intake took off. Intakes logged before D-033 do not
    // know (stockDelta < 0) and give back one, as they took off one.
    static void restore(Store &store, const Intake &intake);
};

} // namespace dollfin

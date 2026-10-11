// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 lillelilje

#include "stockledger.h"

#include "store.h"

#include <QtGlobal>

namespace dollfin {

double StockLedger::perDose(const Stock &stock, const Regimen &regimen)
{
    // Stock units are stable keys (D-032).
    if (stock.unit == QLatin1String("doses"))
        return 1.0;
    return unitsPerDose(regimen);
}

double StockLedger::consume(Store &store, qint64 medicationId, const Regimen &regimen)
{
    bool hasStock = false;
    Stock stock = store.stock(medicationId, &hasStock);
    if (!hasStock)
        return 0.0; // not tracking stock for this medication
    const double taken = qMin(stock.quantity, perDose(stock, regimen));
    if (taken <= 0.0)
        return 0.0;
    stock.quantity -= taken;
    return store.setStock(stock) ? taken : 0.0;
}

void StockLedger::restore(Store &store, const Intake &intake)
{
    const double amount = intake.stockDelta >= 0.0 ? intake.stockDelta : 1.0;
    if (amount <= 0.0)
        return;
    bool hasStock = false;
    Stock stock = store.stock(intake.medicationId, &hasStock);
    if (!hasStock)
        return;
    stock.quantity += amount;
    store.setStock(stock);
}

} // namespace dollfin

#include "supplier.h"
#include "costs.h"
#include "seller.h"
#include <iostream>
#include <ostream>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcothread.h>

IWindowInterface *Supplier::interface = nullptr;
PcoMutex supplierMutex(PcoMutex::Recursive);

Supplier::Supplier(int uniqueId, int fund,
                   std::vector<ItemType> resourcesSupplied)
    : Seller(fund, uniqueId), resourcesSupplied(resourcesSupplied),
      nbSupplied(0) {
  for (const auto &item : resourcesSupplied) {
    stocks[item] = 0;
  }

  interface->consoleAppendText(uniqueId, QString("Supplier Created"));
  interface->updateFund(uniqueId, fund);
}

// buy
int Supplier::request(ItemType it, int qty) {
  supplierMutex.lock();
  int totalCost = getCostPerUnit(it) * qty;

  if (money < totalCost) {
    supplierMutex.unlock();
    return 0; // Not enough funds to proceed
  }

  stocks[it] += qty;
  money -= totalCost;

  // Update the interface immediately after changes
  interface->updateFund(uniqueId, money);
  interface->updateStock(uniqueId, &stocks);

  supplierMutex.unlock();
  return totalCost;
}

void Supplier::run() {
  interface->consoleAppendText(uniqueId, "[START] Supplier routine");

  std::cout << "[Start] Supplier routine" << std::endl;
  while (!PcoThread::thisThread()->stopRequested()) {
    supplierMutex.lock();
    ItemType resourceSupplied = getRandomItemFromStock();
    int employeeSalary =
        getEmployeeSalary(getEmployeeThatProduces(resourceSupplied));
    int totalCost = employeeSalary + getCostPerUnit(resourceSupplied);

    if (money >= totalCost) {
      // import items (simulate delegating importation to employee)
      int result = request(resourceSupplied, 1);
      if (result > 0) {
        interface->consoleAppendText(uniqueId, "[RUN] STILL HAS MONEY");
        // Deduct the salary from the supplier's funds
        money -= employeeSalary;

        nbSupplied++;

        // Update interface with new funds and stocks
        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
      }
    }

    supplierMutex.unlock();
    interface->simulateWork();
  }

  interface->consoleAppendText(uniqueId, "[STOP] Supplier routine");
  std::cout << "[STOP] Supplier routine" << std::endl;
}

std::map<ItemType, int> Supplier::getItemsForSale() { return stocks; }

int Supplier::getMaterialCost() {
  int totalCost = 0;

  if (!resourcesSupplied.size()) {
    return totalCost;
  }

  supplierMutex.lock();
  for (const auto &item : resourcesSupplied) {
    totalCost += getCostPerUnit(item);
  }
  supplierMutex.unlock();

  return totalCost;
}

int Supplier::getAmountPaidToWorkers() {
  return nbSupplied * getEmployeeSalary(EmployeeType::Supplier);
}

void Supplier::setInterface(IWindowInterface *windowInterface) {
  interface = windowInterface;
}

std::vector<ItemType> Supplier::getResourcesSupplied() const {
  return resourcesSupplied;
}

// offer
int Supplier::send(ItemType item, int qty, int bill) {
  // TODO: critical section (stocks[item] is not atomic operation)
  supplierMutex.lock();
  if (stocks.find(item) == stocks.end() || stocks[item] < qty) {
    supplierMutex.unlock();
    return 0;
  }

  // Update funds and stock
  money += bill;
  stocks[item] -= qty;

  // Update the interface immediately after changes
  interface->updateFund(uniqueId, money);
  interface->updateStock(uniqueId, &stocks);

  supplierMutex.unlock();
  // TODO: critical section

  return qty;
}

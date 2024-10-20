#include "supplier.h"
#include "costs.h"
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
  if (stocks.find(it) == stocks.end() || stocks[it] < qty) {
    return 0;
  }

  // TODO: critical section
  supplierMutex.lock();
  int totalCost = getCostPerUnit(it) * qty;

  // Update the stock and the supplier's funds
  stocks[it] -= qty;
  this->money += totalCost;
  // TODO: critical section

  interface->updateStock(uniqueId, &stocks);
  interface->updateFund(uniqueId, money);
  supplierMutex.unlock();

  return totalCost;
}

void Supplier::run() {
  interface->consoleAppendText(uniqueId, "[START] Supplier routine");
    
  std::cout << "[Start] Supplier routine" << std::endl;
    while (!PcoThread::thisThread()->stopRequested()) {
        ItemType resourceSupplied = getRandomItemFromStock();
        int employeeSalary = getEmployeeSalary(getEmployeeThatProduces(resourceSupplied));

        if (money >= employeeSalary) {
            // Lock the mutex before accessing shared resources
            supplierMutex.lock();

            interface->consoleAppendText(uniqueId, "[RUN] STILL HAS MONEY");
            // Deduct the salary from the supplier's funds
            money -= employeeSalary;

            // import items (simulate delegating importation to employee)
            int result = send(resourceSupplied, 1, employeeSalary);
            if (result > 0) {
                nbSupplied++;

                // Update interface with new funds and stocks
                interface->updateFund(uniqueId, money);
                interface->updateStock(uniqueId, &stocks);
            }
            supplierMutex.unlock();
        }

        interface->simulateWork();
    }

    interface->consoleAppendText(uniqueId, "[STOP] Supplier routine");
  std::cout << "[STOP] Supplier routine" << std::endl;
}

std::map<ItemType, int> Supplier::getItemsForSale() { return stocks; }

int Supplier::getMaterialCost() {
  int totalCost = 0;
  for (const auto &item : resourcesSupplied) {
    totalCost += getCostPerUnit(item);
  }
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
  if (money < bill) {
      return 0;
  }

  // Update funds and stock
  // TODO: critical section
  supplierMutex.lock();
  money -= bill;
  stocks[item] += qty;
  supplierMutex.unlock();
  // TODO: critical section

  return qty;
}

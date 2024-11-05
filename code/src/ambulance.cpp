#include "ambulance.h"
#include "seller.h"
#include <pcosynchro/pcothread.h>

IWindowInterface *Ambulance::interface = nullptr;

Ambulance::Ambulance(int uniqueId, int fund,
                     std::vector<ItemType> resourcesSupplied,
                     std::map<ItemType, int> initialStocks)
    : Seller(fund, uniqueId), resourcesSupplied(resourcesSupplied),
      nbTransfer(0) {
  interface->consoleAppendText(uniqueId, QString("Ambulance Created"));

  for (const auto &item : resourcesSupplied) {
    if (initialStocks.find(item) != initialStocks.end()) {
      stocks[item] = initialStocks[item];
    } else {
      stocks[item] = 0;
    }
  }

  interface->updateFund(uniqueId, fund);
}

void Ambulance::sendPatient() {
  if (getFund() > 0) {
    int toPay = getCostPerUnit(ItemType::PatientSick) * DEFAULT_QUANTITY;
    int toReceive = chooseRandomSeller(hospitals)->send(
        ItemType::PatientSick, DEFAULT_QUANTITY, toPay);
    int salary = getEmployeeSalary(EmployeeType::Supplier);

    // has enough money to pay employee salary
    if (toReceive > 0 && getFund() > salary) {
      mutex.lock();

      // Earn revenue from patient transfer
      money += toReceive;
      stocks[ItemType::PatientSick] -= DEFAULT_QUANTITY;
      nbTransfer++;
      // Pay employee salary
      money -= salary;

      mutex.unlock();
    }
  }
}

void Ambulance::run() {
  interface->consoleAppendText(uniqueId, "[START] Ambulance routine");

  while (!PcoThread::thisThread()->stopRequested()) {

    sendPatient();

    interface->simulateWork();

    interface->updateFund(uniqueId, money);
    interface->updateStock(uniqueId, &stocks);
  }

  interface->consoleAppendText(uniqueId, "[STOP] Ambulance routine");
}

std::map<ItemType, int> Ambulance::getItemsForSale() { return stocks; }

int Ambulance::getMaterialCost() {
  int totalCost = 0;
  for (const auto &item : resourcesSupplied) {
    totalCost += getCostPerUnit(item);
  }
  return totalCost;
}

int Ambulance::getAmountPaidToWorkers() {
  return nbTransfer * getEmployeeSalary(EmployeeType::Supplier);
}

int Ambulance::getNumberPatients() { return stocks[ItemType::PatientSick]; }

void Ambulance::setInterface(IWindowInterface *windowInterface) {
  interface = windowInterface;
}

void Ambulance::setHospitals(std::vector<Seller *> hospitals) {
  this->hospitals = hospitals;

  for (Seller *hospital : hospitals) {
    interface->setLink(uniqueId, hospital->getUniqueId());
  }
}

int Ambulance::send(ItemType it, int qty, int bill) { return 0; }

int Ambulance::request(ItemType what, int qty) { return 0; }

std::vector<ItemType> Ambulance::getResourcesSupplied() const {
  return resourcesSupplied;
}

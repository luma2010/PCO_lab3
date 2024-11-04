#include "clinic.h"
#include <iostream>
#include <pcosynchro/pcothread.h>

IWindowInterface *Clinic::interface = nullptr;

Clinic::Clinic(int uniqueId, int fund, std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId), nbTreated(0), resourcesNeeded(resourcesNeeded) {
  interface->updateFund(uniqueId, fund);
  interface->consoleAppendText(uniqueId, "Factory created");

  for (const auto &item : resourcesNeeded) {
    stocks[item] = 0;
  }
}

bool Clinic::verifyResources() {
  for (auto item : resourcesNeeded) {
    if (stocks[item] == 0) {
      return false;
    }
  }
  return true;
}

int Clinic::request(ItemType what, int qty) {
  int bill = getEmployeeSalary(EmployeeType::Nurse) * qty;

  if (bill > 0 && stocks[what] > 0) {
    mutex.lock();
    money += bill;
    stocks[what] -= qty;
    mutex.unlock();

    return bill;
  }

  return 0;
}

void Clinic::treatPatient() {
  // TODO
  int qty = 1;
  int Salary = getEmployeeSalary(EmployeeType::Doctor);
  if (money - Salary >= 0) {
    mutex.lock();
    for (auto &resource : resourcesNeeded) {
      stocks[resource] -= qty;
    }
    mutex.unlock();

    // Temps simulant un traitement
    interface->simulateWork();
    mutex.lock();
    nbTreated += qty;
    stocks[ItemType::PatientHealed] += qty;
    money -= Salary;
    mutex.unlock();
  }

  // TODO

  interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() {
  for (auto &resource : resourcesNeeded) {
    if (resource == ItemType::PatientSick) {
      int qty = 1;
      if (qty * getCostPerUnit(resource) <= money) {
        int bill = chooseRandomSeller(hospitals)->request(resource, qty);
        if (bill > 0) {
          mutex.lock();
          money -= bill;
          stocks[resource] += qty;
          mutex.unlock();
        }
      }
    } else if (resource == ItemType::PatientHealed) {

    } else {
      int qty = 1;
      if (qty * getCostPerUnit(resource) <= money) {
        int bill = chooseRandomSeller(suppliers)->request(resource, qty);
        if (bill > 0) {
          mutex.lock();
          money -= bill;
          stocks[resource] += qty;
          mutex.unlock();
        }
      }
    }
  }
}

void Clinic::run() {
  if (hospitals.empty() || suppliers.empty()) {
    std::cerr << "You have to give to hospitals and suppliers to run a clinic"
              << std::endl;
    return;
  }
  interface->consoleAppendText(uniqueId, "[START] Factory routine");

  while (!PcoThread::thisThread()->stopRequested()) {

    if (verifyResources()) {
      treatPatient();
    } else {
      orderResources();
    }

    interface->simulateWork();

    interface->updateFund(uniqueId, money);
    interface->updateStock(uniqueId, &stocks);
  }
  interface->consoleAppendText(uniqueId, "[STOP] Factory routine");
}

void Clinic::setHospitalsAndSuppliers(std::vector<Seller *> hospitals,
                                      std::vector<Seller *> suppliers) {
  this->hospitals = hospitals;
  this->suppliers = suppliers;

  for (Seller *hospital : hospitals) {
    interface->setLink(uniqueId, hospital->getUniqueId());
  }
  for (Seller *supplier : suppliers) {
    interface->setLink(uniqueId, supplier->getUniqueId());
  }
}

int Clinic::getTreatmentCost() { return 0; }

int Clinic::getWaitingPatients() { return stocks[ItemType::PatientSick]; }

int Clinic::getNumberPatients() {
  return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed];
}

int Clinic::send(ItemType it, int qty, int bill) { return 0; }

int Clinic::getAmountPaidToWorkers() {
  return nbTreated *
         getEmployeeSalary(getEmployeeThatProduces(ItemType::PatientHealed));
}

void Clinic::setInterface(IWindowInterface *windowInterface) {
  interface = windowInterface;
}

std::map<ItemType, int> Clinic::getItemsForSale() { return stocks; }

Pulmonology::Pulmonology(int uniqueId, int fund)
    : Clinic::Clinic(
          uniqueId, fund,
          {ItemType::PatientSick, ItemType::Pill, ItemType::Thermometer}) {}

Cardiology::Cardiology(int uniqueId, int fund)
    : Clinic::Clinic(
          uniqueId, fund,
          {ItemType::PatientSick, ItemType::Syringe, ItemType::Stethoscope}) {}

Neurology::Neurology(int uniqueId, int fund)
    : Clinic::Clinic(
          uniqueId, fund,
          {ItemType::PatientSick, ItemType::Pill, ItemType::Scalpel}) {}

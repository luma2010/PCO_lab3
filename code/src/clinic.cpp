#include "clinic.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcothread.h>

IWindowInterface *Clinic::interface = nullptr;
PcoMutex clinicMutex(PcoMutex::Recursive);

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
  int totalReceived = 0;

  for (Seller *hospital : hospitals) {
    int received = hospital->send(what, qty - totalReceived, 0);
    if (received > 0) {
      // TODO: section critique
      clinicMutex.lock();
      stocks[what] += received;
      totalReceived += received;
      clinicMutex.unlock();
      // TODO: section critique

      QString message =
          QString("Requested %1 %2").arg(received).arg(static_cast<int>(what));
      interface->consoleAppendText(uniqueId, message);

      if (totalReceived >= qty) {
        break;
      }
    }
  }

  return totalReceived;
}

void Clinic::treatPatient() {
  // TODO
  // First, treat the patient
  for (const auto &item : resourcesNeeded) {
    if (item == ItemType::PatientSick) {
      clinicMutex.lock();
      stocks[item]--;
      nbTreated++;
      interface->consoleAppendText(uniqueId, "Clinic has healed a new patient");
      clinicMutex.unlock();

      break;
    }
  }

  // Then, pay the employee
  EmployeeType employee = getEmployeeThatProduces(ItemType::PatientHealed);
  int salary = getEmployeeSalary(employee);

  if (money < salary) {
    interface->consoleAppendText(uniqueId,
                                 "Clinic has no money to pay the employee");
    return;
  }

  // TODO: section critique
  clinicMutex.lock();
  money -= salary;
  interface->updateFund(uniqueId, money);
  clinicMutex.unlock();
  // TODO: section critique

  // Temps simulant un traitement
  interface->simulateWork();

  // TODO

  interface->consoleAppendText(uniqueId, "Clinic have healed a new patient");
}

void Clinic::orderResources() {
  for (const auto &resource : resourcesNeeded) {
    for (auto *supplier : suppliers) {
      int availableQty = 1;
      int cost = supplier->request(resource, availableQty);
      if (cost > 0) {
        clinicMutex.lock();
        this->money -= cost;
        this->stocks[resource] += availableQty;
        clinicMutex.unlock();
        break;
      }
    }
  }

  if (!hospitals.empty()) {
    int qty = 1;
    // take patiens from hospital 0
    int receivedPatients = hospitals[0]->request(ItemType::PatientSick, qty);
    if (receivedPatients > 0) {
      clinicMutex.lock();
      this->stocks[ItemType::PatientSick] += receivedPatients;
      clinicMutex.unlock();
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

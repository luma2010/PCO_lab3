#include "hospital.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcothread.h>

IWindowInterface *Hospital::interface = nullptr;

Hospital::Hospital(int uniqueId, int fund, int maxBeds)
    : Seller(fund, uniqueId), maxBeds(maxBeds), currentBeds(0),
      nbHospitalised(0), nbFree(0) {
  interface->updateFund(uniqueId, fund);
  interface->consoleAppendText(
      uniqueId, "Hospital Created with " + QString::number(maxBeds) + " beds");

  std::vector<ItemType> initialStocks = {ItemType::PatientHealed,
                                         ItemType::PatientSick};

  for (const auto &item : initialStocks) {
    stocks[item] = 0;
  }
}

int Hospital::request(ItemType what, int qty) {
  mutex.lock();
  if (stocks[what] - qty >= 0) {
    int bill = qty * getCostPerUnit(what);
    stocks[what] -= qty;
    money += bill;
    currentBeds -= qty;
    mutex.unlock();
    return bill;
  }
  mutex.unlock();
  return 0;
}

void Hospital::freeHealedPatient() {
  if (stocks[ItemType::PatientHealed] > 0) {
    mutex.lock();
    for (size_t i = 0; i < nbDaysLeft.size(); i++) {
      if (nbDaysLeft[i] > 0) {
        nbDaysLeft[i] -= 1;
      } else {
        stocks[ItemType::PatientHealed] -= 1;
        nbFree++;
        nbDaysLeft.erase(nbDaysLeft.begin() + i);
        currentBeds -= 1;
      }
    }
    // for (auto it = nbDaysLeft.begin(); it != nbDaysLeft.end(); ) {
    //     if (*it > 0) {
    //         (*it) -= 1;
    //         ++it;
    //     } else {
    //         stocks[ItemType::PatientHealed] -= 1;
    //         nbFree++;
    //         currentBeds -= 1;
    //         it = nbDaysLeft.erase(it);  // Erase returns the iterator to the
    //         next element
    //     }
    // }
    mutex.unlock();
  }
}

void Hospital::transferPatientsFromClinic() {
  int qty = 1;
  int bill = chooseRandomSeller(clinics)->request(ItemType::PatientHealed, qty);
  if (bill > 0) {
    if (money - bill * qty >= 0 && maxBeds - currentBeds - qty >= 0) {
      mutex.lock();
      currentBeds += qty;
      stocks[ItemType::PatientHealed] += qty;
      money -= bill;
      nbDaysLeft.push_back(5);
      mutex.unlock();
    }
  }
  // TODO
}

int Hospital::send(ItemType it, int qty, int bill) {
  if (money < qty * getCostPerUnit(it)) {
    return 0;
  } else if (maxBeds - currentBeds - qty >= 0) {
    mutex.lock();
    stocks[it] += qty;
    currentBeds += qty;

    money -= bill;
    nbHospitalised++;
    money -= getEmployeeSalary(EmployeeType::Nurse);

    mutex.unlock();
    return bill;
  }
  return 0;
}

void Hospital::run() {
  if (clinics.empty()) {
    std::cerr
        << "You have to give clinics to a hospital before launching is routine"
        << std::endl;
    return;
  }

  interface->consoleAppendText(uniqueId, "[START] Hospital routine");

  while (!PcoThread::thisThread()->stopRequested()) {
    transferPatientsFromClinic();

    freeHealedPatient();

    interface->updateFund(uniqueId, money);
    interface->updateStock(uniqueId, &stocks);
    interface->simulateWork(); // Temps d'attente
  }

  interface->consoleAppendText(uniqueId, "[STOP] Hospital routine");
}

int Hospital::getAmountPaidToWorkers() {
  return nbHospitalised * getEmployeeSalary(EmployeeType::Nurse);
}

int Hospital::getNumberPatients() {
  return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed] +
         nbFree;
}

std::map<ItemType, int> Hospital::getItemsForSale() { return stocks; }

void Hospital::setClinics(std::vector<Seller *> clinics) {
  this->clinics = clinics;

  for (Seller *clinic : clinics) {
    interface->setLink(uniqueId, clinic->getUniqueId());
  }
}

void Hospital::setInterface(IWindowInterface *windowInterface) {
  interface = windowInterface;
}

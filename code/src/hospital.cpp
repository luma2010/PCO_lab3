/**
 * @author Rocha Ferreira Mário André
 * @author Chollet Florian
 */

#include "hospital.h"
#include "costs.h"
#include "seller.h"
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

// "sell" patient
int Hospital::request(ItemType what, int qty) {
  if (qty == 0) {
    return 0;
  }

  mutex.lock();
  if (stocks[what] - qty >= 0) {
    int bill = qty * getCostPerUnit(what);

    stocks[what] -= qty;
    money += bill;
    // free patient bed
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
    for (auto it = nbDaysLeft.begin(); it != nbDaysLeft.end();) {
      if (*it > 0) {
        (*it) -= 1;
        ++it;
        // have to do this because when modifying the iterator, it becomes
        // invalid (cases segfault when erasing from nbDaysLeft)
      } else {
        stocks[ItemType::PatientHealed] -= DEFAULT_QUANTITY;
        nbFree++;
        currentBeds -= 1;
        it = nbDaysLeft.erase(it); // Remove healed patient with expired days
      }
    }
    mutex.unlock();
  }
}

void Hospital::transferPatientsFromClinic() {
  int bill = chooseRandomSeller(clinics)->request(ItemType::PatientHealed,
                                                  DEFAULT_QUANTITY);
  if (bill > 0) {
    // if has enough money
    if (money - (bill * DEFAULT_QUANTITY) >= 0 &&
        (maxBeds - currentBeds - DEFAULT_QUANTITY) >= 0) {
      mutex.lock();

      currentBeds += DEFAULT_QUANTITY;
      stocks[ItemType::PatientHealed] += DEFAULT_QUANTITY;
      money -= bill;
      // set lifetime of patient to 5 days
      nbDaysLeft.push_back(5);

      mutex.unlock();
    }
  }
}

// receives patient
int Hospital::send(ItemType it, int qty, int bill) {
  if (qty == 0) {
    return 0;
  }

  // Check if hospital can afford the patient costs
  if (money < qty * getCostPerUnit(it)) {
    return 0;
  } else if (maxBeds - currentBeds - qty >= 0) {
    mutex.lock();
    stocks[it] += qty;
    currentBeds += qty;

    // Deduct costs for receiving patients and paying nurses
    money -= bill;
    nbHospitalised++;
    // pay nurses
    money -= getEmployeeSalary(EmployeeType::Nurse);

    mutex.unlock();
    return bill;
  }

  // Insufficient beds to accept more patients
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

#include "hospital.h"
#include "costs.h"
#include <iostream>
#include <pcosynchro/pcothread.h>

PcoMutex hospitalMutex;

IWindowInterface* Hospital::interface = nullptr;

Hospital::Hospital(int uniqueId, int fund, int maxBeds)
    : Seller(fund, uniqueId), maxBeds(maxBeds), currentBeds(0), nbHospitalised(0), nbFree(0)
{
    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Hospital Created with " + QString::number(maxBeds) + " beds");

    std::vector<ItemType> initialStocks = { ItemType::PatientHealed, ItemType::PatientSick };

    for(const auto& item : initialStocks) {
        stocks[item] = 0;
    }
}

int Hospital::request(ItemType what, int qty){
    hospitalMutex.lock();
    if(stocks[what] - qty >= 0){
        int bill = qty*getCostPerUnit(what);
        stocks[what] -= qty;
        money += bill;
        currentBeds -= qty;
        hospitalMutex.unlock();
        return bill;
    }
    hospitalMutex.unlock();
    return 0;
}

void Hospital::freeHealedPatient() {
    if(stocks[ItemType::PatientHealed] > 0){
        hospitalMutex.lock();
        for(size_t i = 0; i < nbDayLeft.size(); i++){
            if(nbDayLeft[i] > 0){
                nbDayLeft[i] -= 1;
            }else{
                stocks[ItemType::PatientHealed] -=1;
                nbFree++;
                nbDayLeft.erase(nbDayLeft.begin()+i);
                currentBeds -= 1;
            }
        }
        hospitalMutex.unlock();
    }

}

void Hospital::transferPatientsFromClinic() {
    int qty = 1;
    int bill = chooseRandomSeller(clinics)->request(ItemType::PatientHealed, qty);
    if(bill > 0){
        if(money - bill*qty >= 0 && maxBeds - currentBeds - qty >= 0){
            hospitalMutex.lock();
            currentBeds +=qty;
            stocks[ItemType::PatientHealed] += qty;
            money -= bill;
            nbDayLeft.push_back(5);
            hospitalMutex.unlock();
        }
    }
    // TODO
}

int Hospital::send(ItemType it, int qty, int bill) {
    if(money < qty*getCostPerUnit(it)){
        return 0;
    }else if(maxBeds - currentBeds - qty >= 0){
        hospitalMutex.lock();
        stocks[it] += qty;
        currentBeds += qty;
        money -= bill;
        nbHospitalised++;
        money -= getEmployeeSalary(EmployeeType::Nurse);
        hospitalMutex.unlock();
        return bill;
    }
    return 0;
}

void Hospital::run()
{
    if (clinics.empty()) {
        std::cerr << "You have to give clinics to a hospital before launching is routine" << std::endl;
        return;
    }

    interface->consoleAppendText(uniqueId, "[START] Hospital routine");

    while (!PcoThread::thisThread()->stopRequested()) {
        transferPatientsFromClinic();

        freeHealedPatient();

        std::cout << "for " << uniqueId << " maxBeds = " << currentBeds << std::endl;

        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
        interface->simulateWork(); // Temps d'attente
    }

    interface->consoleAppendText(uniqueId, "[STOP] Hospital routine");
}

int Hospital::getAmountPaidToWorkers() {
    return nbHospitalised * getEmployeeSalary(EmployeeType::Nurse);
}

int Hospital::getNumberPatients(){
    return stocks[ItemType::PatientSick] + stocks[ItemType::PatientHealed] + nbFree;
}

std::map<ItemType, int> Hospital::getItemsForSale()
{
    return stocks;
}

void Hospital::setClinics(std::vector<Seller*> clinics){
    this->clinics = clinics;

    for (Seller* clinic : clinics) {
        interface->setLink(uniqueId, clinic->getUniqueId());
    }
}

void Hospital::setInterface(IWindowInterface* windowInterface){
    interface = windowInterface;
}

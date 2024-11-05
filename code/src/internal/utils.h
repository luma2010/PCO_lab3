/**
 * @author Rocha Ferreira Mário André
 * @author Chollet Florian
 * Only modified to fix memory leak
 */

#ifndef UTILS_H
#define UTILS_H

#include <QDebug>
#include <QRandomGenerator>
#include <QTextStream>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>
#include <vector>

#include "ambulance.h"
#include "clinic.h"
#include "hospital.h"
#include "supplier.h"

#define NB_SUPPLIER 3
#define NB_CLINICS 3
#define NB_HOSPITALS 2
#define SUPPLIER_FUND 200
#define CLINICS_FUND 300
#define HOSPITALS_FUND 1000

#define INITIAL_SCALPEL 400
#define INITIAL_THERMOMETER 350
#define INITIAL_STETHOSCOPE 600

#define INITIAL_PILL 350
#define INITIAL_SYRINGE 530

#define INITIAL_PATIENT_SICK 900

#define MAX_BEDS_PER_HOSTPITAL 35

std::vector<Ambulance *> createAmbulances(int nbAmbulances, int idStart);
std::vector<Supplier *> createSuppliers(int nbSuppliers, int idStart);
std::vector<Clinic *> createClinics(int nbClinics, int idStart);
std::vector<Hospital *> createHospitals(int nbHospitals, int idStart);

class Utils {
public:
  void externalEndService();
  QString getFinalReport();

private:
  std::vector<Ambulance *> ambulances;
  std::vector<Supplier *> suppliers;
  std::vector<Clinic *> clinics;
  std::vector<Hospital *> hospitals;

  std::vector<std::unique_ptr<PcoThread>> threads;
  std::unique_ptr<PcoThread> utilsThread;

  QString finalReport;

  void endService();

  void run();

  PcoSemaphore semEnd{0};

public:
  Utils(int nbSupplier, int nbClinic, int nbHospital);
  ~Utils() {

    for (size_t i = 0; i < this->ambulances.size(); ++i) {
      Ambulance *ambulance = this->ambulances.at(i);

      if (ambulance != nullptr) {
        ::delete ambulance;
      }
    }

    for (size_t i = 0; i < this->suppliers.size(); ++i) {
      Supplier *supplier = this->suppliers.at(i);

      if (supplier != nullptr) {
        ::delete supplier;
      }
    }

    for (size_t i = 0; i < this->clinics.size(); ++i) {
      Clinic *clinic = this->clinics.at(i);

      if (clinic != nullptr) {
        ::delete clinic;
      }
    }

    for (size_t i = 0; i < this->hospitals.size(); ++i) {
      Hospital *hospital = this->hospitals.at(i);

      if (hospital != nullptr) {
        ::delete hospital;
      }
    }
  }
};

#endif // UTILS_H

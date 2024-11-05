/**
 * @author Rocha Ferreira Mário André
 * @author Chollet Florian
 */

#include "ambulance.h"
#include "fakeinterface.h"
#include "iwindowinterface.h"
#include "supplier.h"
#include "utils.h"
#include <gtest/gtest.h>
#include <iostream>
#include <pcosynchro/pcothread.h>
#include <random>
#include <vector>

void sendPatients(Hospital &hospital, ItemType itemType,
                  std::atomic<int> &totalPaid) {
  int tot = 0;
  for (int i = 0; i < 20000; ++i) {
    int qty = 1;
    // int bill = getCostPerUnit(itemType) * qty;
    int bill = getCostPerUnit(ItemType::PatientSick) * qty;
    if (hospital.send(itemType, qty, bill) > 0) {
      tot += bill;
    }
  }

  totalPaid += tot;
}

void requestPatients(Hospital &hospital, ItemType itemType,
                     std::atomic<int> &totalGained) {
  int tot = 0;
  for (int i = 0; i < 20000; ++i) {
    int qty = 1;
    tot += hospital.request(itemType, qty);
  }

  totalGained += tot;
}

TEST(SellerTest, TestHospitals) {
  const int uniqueId = 0;
  const int initialFund = 20000;
  const unsigned int maxBeds = MAX_BEDS_PER_HOSTPITAL;
  const unsigned int nbThreads = 4;
  int endFund = 0;
  std::atomic<int> totalPaid = 0;
  std::atomic<int> totalGained = 0;

  IWindowInterface *windowInterface = new FakeInterface();
  Hospital::setInterface(windowInterface);

  Hospital hospital(uniqueId, initialFund, maxBeds);

  std::vector<std::unique_ptr<PcoThread>> threads;

  for (unsigned int i = 0; i < nbThreads / 2; ++i) {
    threads.emplace_back(std::make_unique<PcoThread>(
        requestPatients, std::ref(hospital), ItemType::PatientSick,
        std::ref(totalGained)));
    threads.emplace_back(std::make_unique<PcoThread>(
        sendPatients, std::ref(hospital), ItemType::PatientSick,
        std::ref(totalPaid)));
  }

  for (auto &thread : threads) {
    thread->join();
  }

  endFund += hospital.getFund();
  endFund += hospital.getAmountPaidToWorkers();
  endFund += totalPaid;   // <- En test cet argent est perdu
  endFund -= totalGained; // <- En test cette argent vient de nulle part

  EXPECT_EQ(endFund, initialFund);
  EXPECT_GE(hospital.getNumberPatients(), 0);
  EXPECT_LE(hospital.getNumberPatients(), maxBeds);
}

// High volume request patients function for testing
void highVolumeRequestPatients(Hospital &hospital, ItemType itemType,
                               std::atomic<int> &totalGained) {
  int tot = 0;
  for (int i = 0; i < 50000; ++i) { // Increased count for stress testing
    int qty = 1;
    tot += hospital.request(itemType, qty);
  }
  totalGained += tot;
}

// High volume send patients function for testing
void highVolumeSendPatients(Hospital &hospital, ItemType itemType,
                            std::atomic<int> &totalPaid) {
  int tot = 0;
  for (int i = 0; i < 50000; ++i) { // Increased count for stress testing
    int qty = 1;
    int bill = getCostPerUnit(itemType) * qty;
    if (hospital.send(itemType, qty, bill) > 0) {
      tot += bill;
    }
  }
  totalPaid += tot;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(HospitalTest, ConcurrencyStressTest) {
  const int uniqueId = 2;
  const int initialFund = 100000;
  const unsigned int maxBeds = MAX_BEDS_PER_HOSTPITAL;
  const int numThreads = 20;
  std::atomic<int> totalPaid = 0;
  std::atomic<int> totalGained = 0;

  IWindowInterface *windowInterface = new FakeInterface();
  Hospital::setInterface(windowInterface);

  Hospital hospital(uniqueId, initialFund, maxBeds);

  std::vector<std::unique_ptr<PcoThread>> threads;
  for (int i = 0; i < numThreads / 2; ++i) {
    threads.emplace_back(std::make_unique<PcoThread>(
        requestPatients, std::ref(hospital), ItemType::PatientSick,
        std::ref(totalGained)));
    threads.emplace_back(std::make_unique<PcoThread>(
        sendPatients, std::ref(hospital), ItemType::PatientSick,
        std::ref(totalPaid)));
  }

  for (auto &thread : threads) {
    thread->join();
  }

  int endFund = totalPaid - totalGained;
  EXPECT_EQ(endFund, 0);

  // Bounds check for patient numbers
  EXPECT_GE(hospital.getNumberPatients(), 0);
  EXPECT_LE(hospital.getNumberPatients(), maxBeds);
}

TEST(HospitalTest, NegativeAndZeroCases) {
  const int uniqueId = 4;
  const int initialFund = 10000;
  const unsigned int maxBeds = 5;

  IWindowInterface *windowInterface = new FakeInterface();
  Hospital::setInterface(windowInterface);

  Hospital hospital(uniqueId, initialFund, maxBeds);

  // Attempt to send a patient with zero quantity and expect no funds change
  int initialHospitalFunds = hospital.getFund();
  hospital.send(ItemType::PatientSick, 0, 0);
  EXPECT_EQ(hospital.getFund(), initialHospitalFunds);

  std::atomic<int> totalGained = 0;
  requestPatients(hospital, ItemType::PatientSick, totalGained);
  EXPECT_EQ(totalGained, 0);
}

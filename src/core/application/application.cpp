//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "application.h"

MApplication::MApplication() : MObject(){
    name = TEXT("Application");
}

bool MApplication::isRunning() const {
    {return appRunning;}
}

void MApplication::initialise() {

}

void MApplication::run() {

}

void MApplication::cleanup() {

}

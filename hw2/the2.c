#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/time.h>
#include "writeOutput.h"
#include "writeOutput.c"
#include "helper.h"
#include "helper.c"

typedef struct Hub Hub;
typedef struct Sender Sender;
typedef struct Receiver Receiver;
typedef struct Drone Drone;
/* Hub Struct */
struct Hub {
    unsigned int ID;
    unsigned int senderID;
    unsigned int receiverID;

    unsigned int isActiveHub;
    unsigned int isActiveSender;
    unsigned int isActiveReceiver;
    unsigned int incomingPackageStorage;
    unsigned int incomingPackageCount;
    unsigned int reservedCount;
    unsigned int incomingSender;
    PackageInfo* incoming;
    pthread_mutex_t mutexIncoming;
    pthread_cond_t condIncoming;

    unsigned int outgoingPackageStorage;
    unsigned int outgoingPackageCount;
    unsigned int outgoingSender;
    PackageInfo* outgoing;
    pthread_mutex_t mutexOutgoing;
    pthread_cond_t condOutgoing;

    unsigned int chargingSpaceNum;
    unsigned int chargingSpaceCount;

    unsigned int* distances;

    unsigned int hubCount;
    unsigned int droneCount;

    Hub** hubList;
    Sender** senderList;
    Receiver** receiverList;
    Drone** droneList;
};

/* Receiver Struct */
struct Receiver {
    unsigned int ID;
    unsigned int waitTime;
    unsigned int assignedHubId;
    unsigned int hubCount;
    Hub** hubList;
    Sender** senderList;
};

/* Sender Struct */
struct Sender {
    unsigned int ID;
    unsigned int waitTime;
    unsigned int assignedHubId;
    unsigned int numberOfPackages;
    unsigned int hubCount;
    Hub** hubList;
    Receiver** receiverList;
};

/* Drone Struct */
struct Drone {
    unsigned int ID;
    unsigned int travelSpeed;
    unsigned int startingHubId;
    unsigned int maxRange;
    unsigned int currentRange;
    unsigned int isActiveDrone;

    unsigned int call;  //1 for current, two for distanced
    pthread_mutex_t mutexCall;
    pthread_cond_t condCall;

    Hub** hubList;
    Sender** senderList;
    unsigned int hubCount;
    PackageInfo package;
    unsigned int assignedHubId;
};

int existActiveHub(Hub** hubList, unsigned int hubCount){

    int count = 0;

    for (int i = 0; i < hubCount; ++i)
    {
        if(hubList[i]->isActiveHub == 1){
            count = 1;
        }
    }

    return count;
}

int isDroneInHub(Drone** droneList, unsigned int droneCount, unsigned int hubID){
    int index = -1;
    unsigned int range = 0;

    for (int i = 0; i < droneCount; ++i)
    {
        if(droneList[i]->startingHubId == hubID && droneList[i]->call==0){
            if(droneList[i]->currentRange>=range){
                index = i;
                range = droneList[i]->currentRange;
            }
        }
    }
    if(index != -1){
        pthread_mutex_lock(&droneList[index]->mutexCall);
        while(droneList[index]->call!=0){
            pthread_cond_wait(&droneList[index]->condCall,&droneList[index]->mutexCall);
        }
        if(droneList[index]->startingHubId == hubID && droneList[index]->call==0){        
            droneList[index]->call = 3;
            pthread_mutex_unlock(&droneList[index]->mutexCall);
            pthread_cond_signal(&droneList[index]->condCall);
        }else{
            pthread_mutex_unlock(&droneList[index]->mutexCall);
            pthread_cond_signal(&droneList[index]->condCall);
            index = -1;
        }
    }
    return index;
}

void addToArr(PackageInfo package, PackageInfo* packagelist, unsigned int index){
    packagelist[index] = package;
}
void assignDrone(Hub** hubList, unsigned int hubId, Drone** droneList,int index,PackageInfo package, unsigned int call){
    pthread_mutex_lock(&droneList[index]->mutexCall);
    while(droneList[index]->call!=3){
        pthread_cond_wait(&droneList[index]->condCall,&droneList[index]->mutexCall);
    }
    if(droneList[index]->call==3){
        droneList[index]->call = call;
        droneList[index]->package = package;
        hubList[hubId-1]->outgoingPackageCount = hubList[hubId-1]->outgoingPackageCount + 1;
        pthread_mutex_unlock(&droneList[index]->mutexCall);
        pthread_cond_signal(&droneList[index]->condCall);
    }else{
        pthread_mutex_unlock(&droneList[index]->mutexCall);
        pthread_cond_signal(&droneList[index]->condCall);        
    }
}
unsigned int waitForRange(unsigned int maxRange,unsigned int currentRange,unsigned int speed,unsigned int startingHubId,unsigned int receivingHubId,Hub** hubList){
    unsigned int range = currentRange;
    unsigned int distance = hubList[startingHubId-1]->distances[receivingHubId-1];
    distance = distance / speed;
    while(range<distance){
        wait(UNIT_TIME);
        range++;
    }
    if(range>maxRange){
        return maxRange;
    }else{
        return range;
    }
}

unsigned int waitForTravel(unsigned int maxRange,unsigned int currentRange,unsigned int speed,unsigned int startingHubId,unsigned int receivingHubId,Hub** hubList){
    unsigned int range = currentRange;
    unsigned int distance = hubList[startingHubId-1]->distances[receivingHubId-1];
    distance = distance / speed;
    wait(UNIT_TIME*distance);
    range = range-distance;

    return range;
}

int callDrone(unsigned int *distances ,unsigned int hubID, Drone** droneList, unsigned int droneCount, Hub** hubList){
    int index = -1;
    int dis = 9999;
    for (int i = 0; i < droneCount; ++i)
    {
        if(droneList[i]->call == 0 && droneList[i]->startingHubId!=hubID && distances[droneList[i]->startingHubId-1]<dis){
            index = i;
            dis = distances[droneList[i]->startingHubId-1];
        }
    }
    if(index==-1){
        return -1;
    }else{
        pthread_mutex_lock(&droneList[index]->mutexCall);
        while(droneList[index]->call!=0){
            pthread_cond_wait(&droneList[index]->condCall,&droneList[index]->mutexCall);
        }        
        droneList[index]->call = 2;
        droneList[index]->assignedHubId = hubID;
        hubList[hubID-1]->chargingSpaceCount = hubList[hubID-1]->chargingSpaceCount + 1;

        
        pthread_mutex_unlock(&droneList[index]->mutexCall);
        pthread_cond_signal(&droneList[index]->condCall);
        return 1;
    }
}
int isActiveSenderFunc(Hub** hubList,unsigned int hubCount, unsigned int droneCount, Drone** droneList){
    for (int i = 0; i < hubCount; ++i)
    {
        if(hubList[i]->isActiveSender == 1){
            return 1;
        }
        if((hubList[i]->outgoingSender - hubList[i]->outgoingPackageCount)>0){
            return 1;
        }
    }
    for (int i = 0; i < droneCount; ++i)
    {
        /* code */
        if(droneList[i]->call == 1){
           return 1; 
        }
    }
    return 0;
}

void* senderRoutine(void *args){
    Sender* sender = (Sender*)args;
    SenderInfo senderInfo;
    unsigned int receiver_id = 0;
    FillSenderInfo(&senderInfo, sender->ID, sender->assignedHubId, sender->numberOfPackages, NULL);
    WriteOutput(&senderInfo, NULL, NULL, NULL, SENDER_CREATED);
    while(sender->numberOfPackages>0){
        unsigned int recToAssign;
        tekar:
        recToAssign = (rand() % sender->hubCount)+1;
        if(sender->hubList[recToAssign-1]->isActiveHub == 0 || sender->assignedHubId == recToAssign){
            goto tekar;
        }

        for (int i = 0; i < sender->hubCount; ++i)
        {
            if(sender->receiverList[i]->assignedHubId == recToAssign){
                receiver_id = sender->receiverList[i]->ID;
            }
        }
        //wait for deposit
        PackageInfo package;
        FillPacketInfo(&package, sender->ID, sender->assignedHubId, receiver_id, recToAssign);
        pthread_mutex_lock(&sender->hubList[sender->assignedHubId-1]->mutexOutgoing);


        while((sender->hubList[sender->assignedHubId-1]->outgoingSender - sender->hubList[sender->assignedHubId-1]->outgoingPackageCount)>= sender->hubList[sender->assignedHubId-1]->outgoingPackageStorage){
            pthread_cond_wait(&sender->hubList[sender->assignedHubId-1]->condOutgoing,&sender->hubList[sender->assignedHubId-1]->mutexOutgoing);
        }
        addToArr(package,sender->hubList[sender->assignedHubId-1]->outgoing,sender->hubList[sender->assignedHubId-1]->outgoingSender);
        sender->hubList[sender->assignedHubId-1]->outgoingSender = sender->hubList[sender->assignedHubId-1]->outgoingSender + 1;
        pthread_mutex_unlock(&sender->hubList[sender->assignedHubId-1]->mutexOutgoing);
        pthread_cond_signal(&sender->hubList[sender->assignedHubId-1]->condOutgoing);
        FillSenderInfo(&senderInfo, sender->ID, sender->assignedHubId, sender->numberOfPackages, &package);
        WriteOutput(&senderInfo, NULL, NULL, NULL, SENDER_DEPOSITED);
        sender->numberOfPackages = sender->numberOfPackages-1;

        wait(sender->waitTime*UNIT_TIME);
    }
    sender->hubList[sender->assignedHubId-1]->isActiveSender = 0;
    FillSenderInfo(&senderInfo, sender->ID, sender->assignedHubId, sender->numberOfPackages, NULL);
    WriteOutput(&senderInfo, NULL, NULL, NULL, SENDER_STOPPED);


    return NULL;

}

void* receiverRoutine(void *args){
    Receiver* receiver = (Receiver*)args;
    ReceiverInfo receiverInfo;
    FillReceiverInfo(&receiverInfo, receiver->ID, receiver->assignedHubId, NULL);
    WriteOutput(NULL, &receiverInfo, NULL, NULL, RECEIVER_CREATED);
    while(existActiveHub(receiver->hubList,receiver->hubCount)){

        pthread_mutex_lock(&receiver->hubList[receiver->assignedHubId-1]->mutexIncoming);
        
        while((receiver->hubList[receiver->assignedHubId-1]->incomingSender - receiver->hubList[receiver->assignedHubId-1]->incomingPackageCount )<= 0 ){
            pthread_cond_wait(&receiver->hubList[receiver->assignedHubId-1]->condIncoming,&receiver->hubList[receiver->assignedHubId-1]->mutexIncoming);
            if(existActiveHub(receiver->hubList,receiver->hubCount)!= 1){
                break;
            }            
        }

        if(existActiveHub(receiver->hubList,receiver->hubCount)!= 1){
            break;
        }

        //işlemler buraya
        int count = receiver->hubList[receiver->assignedHubId-1]->incomingSender;
        for (int i = receiver->hubList[receiver->assignedHubId-1]->incomingPackageCount; i < count; ++i)
        {

            PackageInfo package;
            FillPacketInfo(&package,receiver->hubList[receiver->assignedHubId-1]->incoming[i].sender_id,receiver->hubList[receiver->assignedHubId-1]->incoming[i].sending_hub_id,receiver->ID,receiver->assignedHubId);
            receiver->hubList[receiver->assignedHubId-1]->incomingPackageCount = receiver->hubList[receiver->assignedHubId-1]->incomingPackageCount + 1;
            FillReceiverInfo(&receiverInfo, receiver->ID, receiver->assignedHubId, &package);
            WriteOutput(NULL, &receiverInfo, NULL, NULL, RECEIVER_PICKUP);
            wait(receiver->waitTime*UNIT_TIME);
        }

        pthread_mutex_unlock(&receiver->hubList[receiver->assignedHubId-1]->mutexIncoming);
    }
    receiver->hubList[receiver->assignedHubId-1]->isActiveReceiver = 0;
    FillReceiverInfo(&receiverInfo, receiver->ID, receiver->assignedHubId, NULL);
    WriteOutput(NULL, &receiverInfo, NULL, NULL, RECEIVER_STOPPED);

    return NULL;
}

// sinya aldıktan sonra receiver beklesin

void* droneRoutine(void *args){
    Drone* drone = (Drone*)args;
    DroneInfo droneInfo;
    FillDroneInfo(&droneInfo, drone->ID, drone->startingHubId, drone->currentRange, NULL, 0);
    WriteOutput(NULL, NULL, &droneInfo, NULL, DRONE_CREATED);
    while(existActiveHub(drone->hubList,drone->hubCount)){
        pthread_mutex_lock(&drone->mutexCall);
        while(drone->call == 0){
            pthread_cond_wait(&drone->condCall,&drone->mutexCall);
            if(existActiveHub(drone->hubList,drone->hubCount)==0){
                break;
            }
        }
        if(existActiveHub(drone->hubList,drone->hubCount)==0){
            break;
        }

        if(drone->call == 1){
            pthread_mutex_lock(&drone->hubList[drone->package.receiving_hub_id-1]->mutexIncoming);
            while(((drone->hubList[drone->package.receiving_hub_id-1]->incomingSender - drone->hubList[drone->package.receiving_hub_id-1]->incomingPackageCount) + drone->hubList[drone->package.receiving_hub_id-1]->reservedCount )>= drone->hubList[drone->package.receiving_hub_id-1]->incomingPackageStorage || drone->hubList[drone->package.receiving_hub_id-1]->chargingSpaceCount>=drone->hubList[drone->package.receiving_hub_id-1]->chargingSpaceNum){
                pthread_cond_wait(&drone->hubList[drone->package.receiving_hub_id-1]->condIncoming,&drone->hubList[drone->package.receiving_hub_id-1]->mutexIncoming);
            }
            drone->hubList[drone->package.receiving_hub_id-1]->reservedCount = drone->hubList[drone->package.receiving_hub_id-1]->reservedCount + 1;
            drone->hubList[drone->package.receiving_hub_id-1]->chargingSpaceCount = drone->hubList[drone->package.receiving_hub_id-1]->chargingSpaceCount + 1;

            drone->currentRange = waitForRange(drone->maxRange,drone->currentRange,drone->travelSpeed,drone->startingHubId,drone->package.receiving_hub_id,drone->hubList);
            PackageInfo package;
            FillPacketInfo(&package,drone->package.sender_id, drone->package.sending_hub_id, drone->package.receiver_id, drone->package.receiving_hub_id);
            FillDroneInfo(&droneInfo, drone->ID, drone->startingHubId, drone->currentRange, &package, 0);
            WriteOutput(NULL, NULL, &droneInfo, NULL, DRONE_PICKUP);
            drone->currentRange = waitForTravel(drone->maxRange,drone->currentRange,drone->travelSpeed,drone->startingHubId,drone->package.receiving_hub_id,drone->hubList);
            

            drone->startingHubId = drone->package.receiving_hub_id;
            drone->hubList[drone->startingHubId-1]->incoming[drone->hubList[drone->startingHubId-1]->incomingSender]=package;
            drone->hubList[drone->startingHubId-1]->incomingSender = drone->hubList[drone->startingHubId-1]->incomingSender + 1;
            drone->hubList[drone->startingHubId-1]->reservedCount = drone->hubList[drone->startingHubId-1]->reservedCount - 1;
            drone->hubList[package.sending_hub_id-1]->chargingSpaceCount = drone->hubList[package.sending_hub_id-1]->chargingSpaceCount - 1;
            drone->call = 0;


            FillPacketInfo(&package,drone->package.sender_id, drone->package.sending_hub_id, drone->package.receiver_id, drone->package.receiving_hub_id);
            FillDroneInfo(&droneInfo, drone->ID, drone->startingHubId, drone->currentRange, &package, 0);
            WriteOutput(NULL, NULL, &droneInfo, NULL, DRONE_DEPOSITED);
            pthread_mutex_unlock(&drone->hubList[drone->package.receiving_hub_id-1]->mutexIncoming);
            pthread_cond_signal(&drone->hubList[drone->startingHubId-1]->condIncoming);

        }else if(drone->call == 2){
            drone->currentRange = waitForRange(drone->maxRange,drone->currentRange,drone->travelSpeed,drone->startingHubId,drone->assignedHubId,drone->hubList);
            FillDroneInfo(&droneInfo, drone->ID, drone->startingHubId, drone->currentRange, NULL, drone->assignedHubId);
            WriteOutput(NULL, NULL, &droneInfo, NULL, DRONE_GOING);
            drone->currentRange = waitForTravel(drone->maxRange,drone->currentRange,drone->travelSpeed,drone->startingHubId,drone->assignedHubId,drone->hubList);
            drone->hubList[drone->startingHubId-1]->chargingSpaceCount = drone->hubList[drone->startingHubId-1]->chargingSpaceCount - 1;
            drone->startingHubId = drone->assignedHubId;
            drone->call = 0;
            FillDroneInfo(&droneInfo, drone->ID, drone->startingHubId, drone->currentRange, NULL, 0);
            WriteOutput(NULL, NULL, &droneInfo, NULL, DRONE_ARRIVED);

        }

        pthread_mutex_unlock(&drone->mutexCall);
        pthread_cond_signal(&drone->condCall);
    }
    drone->isActiveDrone = 0;
    FillDroneInfo(&droneInfo, drone->ID, drone->startingHubId, drone->currentRange, NULL, 0);
    WriteOutput(NULL, NULL, &droneInfo, NULL, DRONE_STOPPED);

    return NULL;

}

void* hubRoutine(void *args){
    Hub* hub = (Hub*)args;
    HubInfo hubInfo;
    int index;
    FillHubInfo(&hubInfo, hub->ID);
    WriteOutput(NULL, NULL, NULL, &hubInfo, HUB_CREATED);
    while(((hub->incomingSender - hub->incomingPackageCount)+hub->reservedCount)>0 || (hub->outgoingSender - hub->outgoingPackageCount)>0 || isActiveSenderFunc(hub->hubList,hub->hubCount,hub->droneCount,hub->droneList) == 1){ //ve aktif sender yoksa
        pthread_mutex_lock(&hub->mutexOutgoing);
        if(hub->isActiveSender == 1 || (hub->outgoingSender - hub->outgoingPackageCount)>0){
            while((hub->outgoingSender - hub->outgoingPackageCount) <= 0 ){
                pthread_cond_wait(&hub->condOutgoing,&hub->mutexOutgoing);
            }
        }else{
            goto receiveonly;
        }
        start:
        index = isDroneInHub(hub->droneList,hub->droneCount,hub->ID); 
        if(index!=-1){

            assignDrone(hub->hubList, hub->ID, hub->droneList,index, hub->outgoing[hub->outgoingPackageCount],1);

        }else{
            /*if(1){
                wait(UNIT_TIME);
                goto start;
            }*/
            if(hub->chargingSpaceCount == 0){
                if(callDrone(hub->distances ,hub->ID, hub->droneList, hub->droneCount,hub->hubList)==-1){
                    wait(UNIT_TIME);
                    goto start;
                }
            }
        }
        receiveonly:
        pthread_mutex_unlock(&hub->mutexOutgoing);
        if((hub->outgoingSender - hub->outgoingPackageCount)<hub->outgoingPackageStorage){
            pthread_cond_signal(&hub->condOutgoing);
        }
        if((hub->incomingSender - hub->incomingPackageCount)>0){
            pthread_cond_signal(&hub->condIncoming);
        }
        if(((hub->incomingSender-hub->incomingPackageCount)+hub->reservedCount)<hub->incomingPackageStorage && hub->chargingSpaceCount<hub->chargingSpaceNum){
            pthread_cond_signal(&hub->condIncoming);
        }
    }
    hub->isActiveHub = 0;
    FillHubInfo(&hubInfo, hub->ID);
    WriteOutput(NULL, NULL, NULL, &hubInfo, HUB_STOPPED);
    while(hub->isActiveReceiver == 1){
        pthread_cond_signal(&hub->condIncoming);
    }
    unsigned int temp = 0;
    if(existActiveHub(hub->hubList,hub->hubCount)==0){
        while(1){
            temp = 0;
            for (int i = 0; i < hub->droneCount; ++i)
            {
                /* code */
                pthread_cond_signal(&hub->droneList[i]->condCall);
                if(hub->droneList[i]->isActiveDrone == 0){
                    temp++;
                }
            }
            if(temp == hub->droneCount){
                break;
            }
        }
    }

    return NULL;

}





int main() {
    /* Threads */
    pthread_t *hubThreadList, *senderThreadList, *receiverThreadList, *droneThreadList;
    /* Count variables */
    unsigned int hubCount,droneCount;

    scanf("%u", &hubCount);

    Hub** hubList = malloc(hubCount*sizeof(Hub*));
    hubThreadList = malloc(hubCount*(sizeof(pthread_t)));

    for (unsigned int i = 0; i < hubCount; i++) {
        Hub *hub = (Hub *)malloc(sizeof(Hub));

        unsigned int incomingPackageStorage, outgoingPackageStorage, chargingSpaceNum;
        scanf("%u %u %u", &incomingPackageStorage, &outgoingPackageStorage, &chargingSpaceNum);
        hub->incomingPackageStorage = incomingPackageStorage;
        hub->outgoingPackageStorage = outgoingPackageStorage;
        hub->chargingSpaceNum = chargingSpaceNum;
        hub->chargingSpaceCount = 0;
        hub->ID = i+1;
        hub->distances = malloc(hubCount*sizeof(int));
        hub->incomingPackageCount = 0;
        hub->outgoingPackageCount = 0;
        hub->outgoingSender = 0;
        hub->incoming = malloc(1000*sizeof(PackageInfo));
        hub->outgoing = malloc(1000*sizeof(PackageInfo));
        hub->isActiveSender = 1;
        hub->isActiveReceiver = 1;
        hub->isActiveHub = 1;
        hub->reservedCount = 0;
        pthread_mutex_init(&hub->mutexIncoming,NULL);
        pthread_mutex_init(&hub->mutexOutgoing,NULL);
        pthread_cond_init(&hub->condIncoming,NULL);
        pthread_cond_init(&hub->condOutgoing,NULL);
        for (unsigned int j = 0; j < hubCount; j++) {
            scanf("%u", &hub->distances[j]);

        }
        hubList[i] = hub;
    }



    Sender** senderList = malloc(hubCount*sizeof(Sender*));
    senderThreadList = malloc(hubCount*sizeof(pthread_t));

    for (unsigned int i = 0; i < hubCount; i++) {
        Sender *sender = (Sender *)malloc(sizeof(Sender));

        unsigned int waitTime, assignedHubId, numberOfPackages;
        scanf("%u %u %u", &waitTime, &assignedHubId, &numberOfPackages);
        sender->waitTime = waitTime;
        sender->assignedHubId = assignedHubId;
        sender->numberOfPackages = numberOfPackages;
        sender->ID = i+1;
        sender->hubList = hubList;
        sender->hubCount = hubCount;
        hubList[assignedHubId-1]->senderID = sender->ID;
        senderList[i] = sender;
    }


    Receiver** receiverList = malloc(hubCount*sizeof(Receiver*));
    receiverThreadList = malloc(hubCount*sizeof(pthread_t));

    for (unsigned int i = 0; i < hubCount; i++) {
        Receiver *receiver = (Receiver *)malloc(sizeof(Receiver));

        unsigned int waitTime, assignedHubId;
        scanf("%u %u", &waitTime, &assignedHubId);
        receiver->waitTime = waitTime;
        receiver->assignedHubId = assignedHubId;
        receiver->ID = i+1;
        receiver->hubList = hubList;
        receiver->hubCount = hubCount;
        hubList[assignedHubId-1]->receiverID = receiver->ID;
        receiverList[i] = receiver;
    }

    for (unsigned int i = 0; i < hubCount; i++) {
        senderList[i]->receiverList = receiverList;
        hubList[i]->hubList = hubList;
    }
    for (unsigned int i = 0; i < hubCount; i++) {
        receiverList[i]->senderList = senderList;
    }
    for (unsigned int i = 0; i < hubCount; i++) {
        hubList[i]->receiverList = receiverList;
    }
    for (unsigned int i = 0; i < hubCount; i++) {
        hubList[i]->senderList = senderList;
    }

    scanf("%u", &droneCount);


    Drone** droneList = malloc(droneCount*sizeof(Drone*));
    droneThreadList = malloc(droneCount*sizeof(pthread_t));

    for (unsigned int i = 0; i < droneCount; i++) {
        Drone *drone = (Drone *)malloc(sizeof(Drone));

        unsigned int travelSpeed, startingHubId, maxRange;
        scanf("%u %u %u", &travelSpeed, &startingHubId, &maxRange);
        drone->travelSpeed = travelSpeed;
        drone->startingHubId = startingHubId;
        hubList[startingHubId-1]->chargingSpaceCount = hubList[startingHubId-1]->chargingSpaceCount + 1;
        drone->maxRange = maxRange;
        drone->ID = i+1;
        drone->assignedHubId = 0;
        drone->isActiveDrone = 1;
        droneList[i] = drone;
    }

    for (unsigned int i = 0; i < hubCount; i++) {
        hubList[i]->droneList = droneList;
        hubList[i]->hubCount = hubCount;
        hubList[i]->droneCount = droneCount;

    }
    for (unsigned int i = 0; i < droneCount; i++) {
        droneList[i]->senderList = senderList;
        droneList[i]->hubList = hubList;
        droneList[i]->hubCount = hubCount;
        droneList[i]->call = 0;
    }


    InitWriteOutput();

    for (unsigned int i = 0; i < hubCount; ++i){
        pthread_create(&hubThreadList[i], NULL, hubRoutine, (void *)hubList[i]);
    }

    for (unsigned int i = 0; i < hubCount; ++i){
        pthread_create(&senderThreadList[i], NULL, senderRoutine, (void *)senderList[i]);
    }

    for (unsigned int i = 0; i < hubCount; ++i){
        pthread_create(&receiverThreadList[i], NULL, receiverRoutine, (void *)receiverList[i]);
    }

    for (unsigned int i = 0; i < droneCount; ++i){
        pthread_create(&droneThreadList[i], NULL, droneRoutine, (void *)droneList[i]);
    }


        /* Join threads */
    for (unsigned int i = 0; i < hubCount; ++i){
        pthread_join(hubThreadList[i], NULL);
    }
    free(hubThreadList);

    for (unsigned int i = 0; i < hubCount; ++i){
        pthread_join(senderThreadList[i], NULL);
    }
    free(senderThreadList);

    for (unsigned int i = 0; i < hubCount; ++i){
        pthread_join(receiverThreadList[i], NULL);
    }
    free(receiverThreadList);

    for (unsigned int i = 0; i < droneCount; ++i){
        pthread_join(droneThreadList[i], NULL);
    }
    free(droneThreadList);


    for (int a = 0; a < hubCount; ++a){
        pthread_mutex_destroy(&hubList[a]->mutexIncoming);
        pthread_mutex_destroy(&hubList[a]->mutexOutgoing);
        pthread_cond_destroy(&hubList[a]->condIncoming);
        pthread_cond_destroy(&hubList[a]->condOutgoing);
        free(hubList[a]);
    }
    free(hubList);
    for (int b = 0; b < hubCount; ++b){
        free(senderList[b]);
    }
    free(senderList);
    for (int c = 0; c < hubCount; ++c){
        free(receiverList[c]);
    }
    free(receiverList);
    for (int d = 0; d < droneCount; ++d){
        free(droneList[d]);
    }
    free(droneList);




    return 0;
}



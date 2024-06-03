#include <stdio.h>
#include <stdlib.h>

typedef enum {
    TYPE_HONDA,
    TYPE_KIA
} CarType;

typedef enum {
    TYPE_HONDA_CIVIC,
    TYPE_HONDA_ACCORD
} HondaType;


typedef struct Car {
    int wheels, doors;
    CarType type;
} Car;

typedef struct Honda {
    Car car;
    HondaType type;
    int honda_specific;
} Honda;

typedef struct HondaCivic {
    Honda honda;
    int civic_specific;
} HondaCivic;

typedef struct HondaAccord {
    Honda honda;
    int accord_specific;
} HondaAccord;

typedef struct Kia {
    Car car;
    int k_specific;
} Kia;


Car create_car(int wheels, int doors) {
    return (Car){wheels, doors};
}

Honda create_honda(int h_specific) {
    Honda honda;
    honda.car = create_car(4, 4);
    honda.car.type = TYPE_HONDA;
    honda.type = -1;
    honda.honda_specific = h_specific;

    return honda;
}

HondaCivic create_civic(int c_specific) {
    HondaCivic civic;
    civic.honda = create_honda(0);
    civic.honda.type = TYPE_HONDA_CIVIC;

    return civic;
}

HondaAccord create_accord(int a_specific) {
    HondaAccord accord;
    accord.honda = create_honda(1);
    accord.honda.type = TYPE_HONDA_ACCORD;

    return accord;
}

Kia create_kia(int k_specific) {
    Kia kia;
    kia.car = create_car(4, 4);
    kia.car.type = TYPE_KIA;
    kia.k_specific = k_specific;

    return kia;
}

void honda_vroom(void *honda_voidptr) {
    Honda *honda = (Honda *)honda_voidptr;

    switch (honda->type) {
        case TYPE_HONDA_CIVIC:
            printf("Civic vroom!! \n");
            break;
        case TYPE_HONDA_ACCORD:
            printf("Accord vroom!! \n");
            break;
        default:
            printf("Honda vroom!! \n");
    }
}

void car_vroom(void *car_voidptr) {
    Car *car = (Car *)car_voidptr;

    switch (car->type) {
        case TYPE_KIA:
            printf("Kia vroom!! \n");
            break;
        case TYPE_HONDA:
            honda_vroom(car_voidptr);
            break;
        default:
            printf("Car vroom!! \n");
            break;
    }
}

int main() {
    Kia kia  = create_kia(1);

    HondaCivic civic = create_civic(2);

    Honda honda = create_honda(3);

    car_vroom(&kia);

    car_vroom(&civic);

    car_vroom(&honda);
}
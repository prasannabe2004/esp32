#include <stdio.h>
#include <string.h>

typedef struct Person {
    char firstname[20];
    char lastname[20];
    int age;
    void (*doWork)(char *data);
}Person;

void updatePerson(Person *p, char *data)
{
    strcpy(p->firstname, "data received1");
    strcpy(p->lastname, "data receiced2");
    p->age = 40;
    p->doWork = doWork;
}

void connectAndGetInfo(char *url, void (*ptr)(char *data))
{
    // connect to wifi
    // connect to endpoint
    // allocate mem
    char *data = (char*) malloc(1024);
    //fill buffer
    ptr(data);
    // do work
    // free mem
    free(data);
}
void doWork(char *data)
{
    Person person;
    updatePerson(&person, data);
    printf("%s %s %d\n", person.firstname, person.lastname, person.age);
}

void app_main(void)
{
    connectAndGetInfo("http://www.example.com", doWork);
}
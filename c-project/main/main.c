#include <stdio.h>
#include <string.h>

typedef struct Person {
    char firstname[20];
    char lastname[20];
    int age;
}Person;

void updatePerson(Person *p)
{
    strcpy(p->firstname, "Prasanna");
    strcpy(p->lastname, "Padmanaban");
    p->age = 40;
}

void exclaimIt(char *phrase)
{
    strcat(phrase, "!");
}
void app_main(void)
{
    Person person;
    strcpy(person.firstname, "Bob");
    strcpy(person.lastname, "Fisher");
    person.age = 54;
    char phrase[20] = {"Hello world"};
    exclaimIt(phrase);
    printf("%s\n", phrase);

    printf("%s %s %d\n", person.firstname, person.lastname, person.age);
    updatePerson(&person);
    printf("%s %s %d\n", person.firstname, person.lastname, person.age);
}

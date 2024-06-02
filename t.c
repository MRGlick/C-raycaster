#include <stdio.h>

struct post {
    
    char name[100];
    int likes;
};

int main() {

    struct post posts[5];

    for (int i=0; i<5; i++) {
        printf("Enter Name: ");
        scanf("%s", &posts[i].name);

        printf("Enter likes: ");
        scanf("%d" ,&posts[i].likes);
    }

    int index = 0;

    for (int i=0; i<5; i++) {
        if (posts[i].likes < posts[index].likes) {
            index = i;
        }
    }

    printf("name of the post: %s", posts[index].name);

}

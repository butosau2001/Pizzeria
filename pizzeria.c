#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "pizzeria.h"
#include "queue.h"
#include "helper.h"

void *garcom_func(void *arg) {
    pizza_t *pizza = (pizza_t *) arg;
    garcom_chamar();
    pthread_mutex_unlock(&espaco_livre);
    garcom_entregar(pizza);
    sem_post(&garcom);
    pthread_exit(NULL);
}

void *pizzaiolo_func(void * arg) {
    while (1) {
        pthread_mutex_lock(&mutex_pizzaiolo);
        pizzaiolo_ocupado++;
        pthread_mutex_unlock(&mutex_pizzaiolo);
        sem_wait(&pedidos);
        if (aberto == 0 && pizzeria.n_mesas == pizzeria.n_mesas_disponiveis) {
            break;
        }
        pedido_t *pedido = (pedido_t *) queue_wait(&smart_deck);
        sem_wait(&pizzaiolos_disponiveis);
        pizza_t *pizza = pizzaiolo_montar_pizza(pedido);
        sem_init(&pizza->pronto, 0, 0);
        sem_wait(&forno);
        pthread_mutex_lock(&pa_de_pizza);
        pizzaiolo_colocar_forno(pizza);
        pthread_mutex_unlock(&pa_de_pizza);
        sem_wait(&pizza->pronto);
        pizzaiolo_retirar_forno(pizza);
        sem_post(&forno);
        pthread_mutex_lock(&espaco_livre);
        pthread_mutex_init(&pizza->pegador, NULL);
        sem_post(&pizzaiolos_disponiveis);
        pthread_t garcom_thread;
        pthread_create(&garcom_thread, NULL, garcom_func, (void *) pizza);
        pthread_detach(garcom_thread);
    }
    printf("saiu do while\n");
    pthread_exit(NULL);
}

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos)
{
    pizzeria.tam_forno = tam_forno;
    pizzeria.n_pizzaiolos = n_pizzaiolos;
    pizzeria.n_mesas = n_mesas;
    pizzeria.n_mesas_disponiveis = n_mesas;
    pizzeria.n_garcons = n_garcons;
    pizzeria.tam_deck = tam_deck;
    pizzeria.n_grupos = n_grupos;

    aberto = 1;

    sem_init(&garcom, 0, n_garcons);
    sem_init(&forno, 0, tam_forno);
    sem_init(&mesas_sem, 0, n_mesas);
    sem_init(&pizzaiolos_disponiveis, 0, n_pizzaiolos);
    sem_init(&pedidos, 0, 0);

    pthread_mutex_init(&mutex_mesa, NULL);
    pthread_mutex_init(&mutex_n_mesas, NULL);
    pthread_mutex_init(&pa_de_pizza, NULL);
    pthread_mutex_init(&espaco_livre, NULL);
    pthread_mutex_init(&mutex_pizzaiolo, NULL);

    pizzaiolos = malloc(n_pizzaiolos * sizeof(pthread_t));
    
    queue_init(&smart_deck, tam_deck);

    for (int i = 0; i < n_pizzaiolos; i++) {
        pthread_create(&pizzaiolos[i], NULL, pizzaiolo_func, NULL);
    }   
}

void pizzeria_close()
{
    aberto = 0;
    while (1) {
        if (pizzeria.n_mesas == pizzeria.n_mesas_disponiveis) {
            for (int i = 0; i < pizzaiolo_ocupado; i++) {
                printf("criando pedido fake");
                queue_push_back(&smart_deck, (pedido_t *) NULL);
            }
            break;
        }
    }

    for (int i = 0; i < pizzeria.n_pizzaiolos; i++) {
        pthread_join(pizzaiolos[i], NULL);
    }
}

void pizzeria_destroy()
{
    sem_destroy(&garcom);
    sem_destroy(&forno);
    sem_destroy(&mesas_sem);
    sem_destroy(&pizzaiolos_disponiveis);
    sem_destroy(&pedidos);

    pthread_mutex_destroy(&espaco_livre);
    pthread_mutex_destroy(&mutex_n_mesas);
    pthread_mutex_destroy(&pa_de_pizza);
    pthread_mutex_destroy(&mutex_mesa);
    pthread_mutex_destroy(&mutex_pizzaiolo);

    queue_destroy(&smart_deck);
}

void pizza_assada(pizza_t *pizza)
{
    sem_post(&pizza->pronto);
}

int pegar_mesas(int tam_grupo)
{
    int mesas = ceil((double) tam_grupo / 4);
    while (1) {
        pthread_mutex_lock(&mutex_mesa);
        if (aberto == 1) {
            if (mesas < pizzeria.n_mesas_disponiveis) {
                for (int i = 0; i < mesas; i++) {
                    sem_wait(&mesas_sem);
                }
                pthread_mutex_lock(&mutex_n_mesas);
                pizzeria.n_mesas_disponiveis -= mesas;
                pthread_mutex_unlock(&mutex_n_mesas);
                pthread_mutex_unlock(&mutex_mesa);
                return 0;
            }
        } else if (aberto == 0) {
            pthread_mutex_unlock(&mutex_mesa);
            return -1;
        }
    }
}

void garcom_tchau(int tam_grupo)
{
    int mesas = ceil((double) tam_grupo / 4);
    sem_post(&garcom);
    pthread_mutex_lock(&mutex_n_mesas);
    pizzeria.n_mesas_disponiveis += mesas;
    printf("mesas disponiveis: %d\n", pizzeria.n_mesas_disponiveis);
    pthread_mutex_unlock(&mutex_n_mesas);
    for (int i = 0; i < mesas; i++) {
        sem_post(&mesas_sem);
    }
}

void garcom_chamar()
{
    sem_wait(&garcom);
}

void fazer_pedido(pedido_t *pedido)
{
    queue_push_back(&smart_deck, pedido);
    sem_post(&pedidos);
}

int pizza_pegar_fatia(pizza_t *pizza)
{
    pthread_mutex_lock(&pizza->pegador);
    if (pizza->fatias <= 0) {
        pthread_mutex_unlock(&pizza->pegador);
        return -1;
    }
    pizza->fatias--;
    pthread_mutex_unlock(&pizza->pegador);
    return 0;
}

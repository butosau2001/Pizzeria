#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include "pizzeria.h"
#include "queue.h"
#include "helper.h"

void pizzeria_init(int tam_forno, int n_pizzaiolos, int n_mesas,
                   int n_garcons, int tam_deck, int n_grupos)
{
    pizzeria.tam_forno = tam_forno;
    pizzeria.n_pizzaiolos = n_pizzaiolos;
    pizzeria.n_mesas = n_mesas;
    pizzeria.n_garcons = n_garcons;
    pizzeria.tam_deck = tam_deck;
    pizzeria.n_grupos = n_grupos;

    aberto = 1;

    sem_init(&garcom, 0, n_garcons);
    sem_init(&pizzaiolo, 0, n_pizzaiolos);
    sem_init(&forno, 0, tam_forno);

    pthread_mutex_init(&mutex_mesa, NULL);
    pthread_mutex_init(&pa_de_pizza, NULL);

    pthread_create(&cozinha, NULL, cozinha_func, NULL);

    queue_init(&smart_deck, tam_deck);
}

void *cozinha_func(void *args) {
    while (aberto == 1) {
        sem_wait(&pizzaiolo);
        pedido_t *pedido = (pedido_t *) queue_wait(&smart_deck);
        pizza_t * pizza = pedido_montar_pizza(pedido);

    }
    return NULL;
}

void pizzeria_close()
{
    aberto = 0;
}

void pizzeria_destroy()
{
    sem_destroy(&garcom);
    sem_destroy(&pizzaiolo);
    pthread_mutex_destroy(&pa_de_pizza);
    pthread_mutex_destroy(&mutex_mesa);
    queue_destroy(&smart_deck);
}

void pizza_assada(pizza_t *pizza)
{
}

int pegar_mesas(int tam_grupo)
{
    int mesas = ceil(tam_grupo / 4);
    if (pizzeria.n_mesas < mesas || aberto == 0)
        return -1;
    pthread_lock(&mutex_mesa);
    pizzeria.n_mesas -= mesas;
    pthread_unlock(&mutex_mesa);
    pizzeria.n_mesas -= mesas;
    return 0;
}

void garcom_tchau(int tam_grupo)
{
    int mesas = ceil(tam_grupo / 4);
    pthread_lock(&mutex_mesa);
    pizzeria.n_mesas += mesas;
    pthread_unlock(&mutex_mesa);
    pizzeria.n_mesas += mesas;
}

void garcom_chamar()
{
    sem_wait(&garcom);
}

void fazer_pedido(pedido_t *pedido)
{
    queue_push_back(&smart_deck, pedido);
}

int pizza_pegar_fatia(pizza_t *pizza)
{
    if (pizza->fatias == 0) {
        return -1;
    }
    pthread_lock(&pizza->pegador);
    pizza->fatias--;
    pthread_unlock(&pizza->pegador);
    if (pizza->fatias >= 0) {
        return 0;
    }
}

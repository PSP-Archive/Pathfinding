//---------------------------------------------------------------------------
// Copyright (c) PJEFF 
// pour les cours sur Devsgen
//--------------------------------------------------------------------------- 
//
//   ai  -      Gestion de l'intelligence artificiel pathfinding
//
// ---------------------------------------------------------------------------

#include <oslib/oslib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>

PSP_MODULE_INFO("AI pathfing", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);


int PATHFINDER_NB_NOEUDS = 0;

int SAVE_NOEUDS_DEBUG = 0;

int	* Walkability=NULL;		//matrice de la walkability pour les collisions


//structure de noeud pour le PathFinding
typedef struct Noeud
{
	int x,y;					//coordonnées dans la matrice 30x17
	float CostFromStart,		//coût du chemin entre Start et ce noeud
		  CostToGoal;			//coût du chemin entre ce noeud et Goal
	struct Noeud * Parent,		//pointeur vers le noeud parent
				 * Suivant,		//pointeur vers le noeud suivant, dans la liste
				 * Precedent;	//pointeur vers le noeud précédent, dans la liste
}Noeud;



typedef struct s_point
{
    int x, y;
}Point_T;



typedef struct s_character
{
    int x, y;

   int Approche;						//capacité d'approche d'un perso
   int Approche_Origine;				//capacité d'approche d'origine du monstre
   Point_T *Defense;					//sprites de défense (coord haut gauche, même largeur et hauteur qu'un sprite normal)
   Point_T *Derniere_Position;			//coord de la case où le monstre a été enregistré pour la dernière fois
   unsigned int * Vetements_Textures;	//liste des textures associées aux vêtements du monstre
   int Dernier_Changement_Orientation;	//date du dernier changement d'orientation du monstre
   int Dernier_Calcul_PathFinding;		//date du dernier calcul de PathFinding pour atteindre le PJ le plus proche
   int Dernier_Changement_Position;		//chronomètre permettant de voir si un monstre est bloqué sur la même case ou non
   Point_T *Chiffres[3];				//caractéristiques des chiffres des dégats: x->sens (1->monter,0->descendre), y->hauteur actuelle du chiffre
   Noeud * Open;						//liste des noeuds à inspecter (pathfinding)
   Noeud * Closed;						//liste des noeuds déjà examinés (pathfinding)
   Noeud * Cible;						//prochain noeud du chemin à atteindre pour atteindre la cible finale


}character_t ;

character_t   *Perso1;
character_t   *Perso2;



/*
=================
character_new

Alloue la memoire necessaire pour le personnage
=================
*/
character_t *character_new (void)
{
    character_t *New;
    
    New = (character_t *)calloc (1, sizeof (character_t));

   if (!New){
	if (New) free(New);
	return NULL;
	}

        
    return New;
}


/*
=================
character_setPoss

applique une possition depard au personnage
=================
*/
void character_setPoss(character_t *character, int x, int y)
{
	character->x = x;
	character->y = y;
}





/*
=================
AddLog

enregistre toute valeur dans le fichier debugNoeud.txt
=================
*/
void AddLog (const char *fmt, ...)
{
    va_list ap;
    FILE *handle = fopen ("debugNoeud.txt", "a");

    if (handle)
    {
        va_start (ap, fmt);
        vfprintf (handle, fmt, ap);
        va_end (ap);
        fprintf (handle, "\r\n");
        fclose (handle);
    }
}



/*
=================
PathCostEstimate

Distance entre 2 points
=================
*/
float PathCostEstimate(float x1,float y1,float x2,float y2)
{

	int h = x1 - x2;
   int v = y1 - y2;
   return(h*h + v*v);

}




/*
=================
Rechercher_Noeud

fonction recherchant ToFind dans Liste. Si trouvé, retourne le noeud équivalant situé dans la liste
=================
*/
Noeud * Rechercher_Noeud(Noeud * ToFind,Noeud * Liste)
{

	Noeud * Temp = Liste; //permet de parcourir la liste

	while (Temp!=NULL)
	{
		if (Temp->x==ToFind->x && Temp->y==ToFind->y) return Temp;
		Temp=Temp->Suivant;
	}
	return NULL;
}


/*
=================
CoutEntre2Points

fonction calculant le coût entre deux noeuds (donc entre deux cases de la map) peut etre completer et alourdi pour diverse raison
=================
*/
float CoutEntre2Points(Noeud * N1,Noeud * N2)
{
	return 1.0f;
}




/*
=================
Examiner_Voisin

fonction examinant les cases voisine situé à la position passée en paramètre
=================
*/
void Examiner_Voisin(character_t * Perso,Noeud * Exam,int x,int y,int xGoal,int yGoal)
{
	

	if (x<0 || y<0 || x>480 || y>272)	return;	//on sort de la matrice -> retour en arrière
	if (Walkability[30*y + x])	return;	//obstacle -> retour en arrière

	float NewCost;		//coût depuis le départ jusqu'au nouveau noeud
	Noeud * NewNoeud,	//noeud à examiner
		  * Temp;		//sert à la recherche d'un noeud déjà existant

	Temp = NULL;

	NewNoeud=(Noeud *)malloc(sizeof(Noeud));
	NewNoeud->x=x;
	NewNoeud->y=y;
	NewNoeud->Parent=Exam;
	NewNoeud->Precedent=NULL;
	NewNoeud->Suivant=NULL;
	NewNoeud->CostFromStart=0.0f;
	NewNoeud->CostToGoal=PathCostEstimate(x,y,xGoal,yGoal);
	PATHFINDER_NB_NOEUDS++;

	NewCost=Exam->CostFromStart+CoutEntre2Points(Exam,NewNoeud);
	//on regarde si le noeud à déjà été répertorié ou si on a trouvé un plus court chemin pour l'atteindre
	//recherche de NewNoeud dans Open
	Temp=Rechercher_Noeud(NewNoeud,Perso->Open);
	//si NewNoeud a déjà été répertorié, on regarde si on a trouvé un meilleur chemin
	if (Temp!=NULL)
	{
		if (Temp->CostFromStart <= NewCost)
		{
			//aucune amélioration: on retourne en arrière
			free(NewNoeud);
			return;
		}
		else	//sinon supprime Temp
		{
			if (Temp->Precedent==NULL)
			{
				Perso->Open=Temp->Suivant;
				Perso->Open->Precedent=NULL;
			}
			else
			{
				Temp->Precedent->Suivant=Temp->Suivant;
				if (Temp->Suivant!=NULL)	Temp->Suivant->Precedent=Temp->Precedent;
			}
			free(Temp);
		}
	}
	else
	{
		//recherche de NewNoeud dans Closed
		Temp=Rechercher_Noeud(NewNoeud,Perso->Closed);
		//si NewNoeud a déjà été répertorié, on regarde si on a trouvé un meilleur chemin
		if (Temp!=NULL)
		{
			if (Temp->CostFromStart <= NewCost)
			{
				//aucune amélioration: on retourne en arrière
				free(NewNoeud);
				return;
			}
			else	//sinon supprime Temp
			{
				if (Temp->Precedent==NULL)
				{
					Perso->Closed=Temp->Suivant;
					Perso->Closed->Precedent=NULL;
				}
				else
				{
					Temp->Precedent->Suivant=Temp->Suivant;
					if (Temp->Suivant!=NULL)	Temp->Suivant->Precedent=Temp->Precedent;
				}
				free(Temp);
			}
		}
		else
		{
			//NewNoeud est un nouveau noeud, ou on a trouvé un meilleur chemin pour l'atteindre
			NewNoeud->CostFromStart=NewCost;
			NewNoeud->CostToGoal=PathCostEstimate(NewNoeud->x,NewNoeud->y,xGoal,yGoal);

			//on ajoute NewNoeud à la fin de Open
			if (Perso->Open==NULL)	Perso->Open=NewNoeud;
			else
			{
				if (Perso->Open==NULL)
				{
					NewNoeud->Precedent=NULL;
					NewNoeud->Suivant=NULL;
					Perso->Open=NewNoeud;
				}
				else
				{
					Temp=Perso->Open;
					while (Temp->Suivant!=NULL)	Temp=Temp->Suivant;
					Temp->Suivant=NewNoeud;
					NewNoeud->Precedent=Temp;
					NewNoeud->Suivant=NULL;
				}
			}
		}
	}
}




/*
=================
AStar_Search

Algorithme de recherche pour les noeuds
=================
*/
bool AStar_Search(character_t * Perso,int xStart,int yStart,int xGoal,int yGoal)
{
	Noeud * Exam,	//noeud à examiner
		  * Temp;

	//création du premier noeud dans Open
	Perso->Open=(Noeud *)malloc(sizeof(Noeud));
	Perso->Open->x=xStart;
	Perso->Open->y=yStart;
	Perso->Open->Parent=NULL;
	Perso->Open->Precedent=NULL;
	Perso->Open->Suivant=NULL;
	Perso->Open->CostFromStart=0.0f;
	Perso->Open->CostToGoal=PathCostEstimate(xStart,yStart,xGoal,yGoal);
	Perso->Closed=NULL;
	Perso->Cible=NULL;
	PATHFINDER_NB_NOEUDS++;

	//on lance l'algorithme A*
	while (Perso->Open != NULL)
	{
		//on récupère le noeud de plus faible poids à examiner
		Temp=Perso->Open;
		Exam=Perso->Open;
		while (Temp!=NULL)
		{
			if (Temp->CostFromStart<Exam->CostFromStart)	Exam=Temp;
			Temp=Temp->Suivant;
		}

		//si le noeud est le point d'arrivée, on a terminé la recherche
		if (Exam->x==xGoal && Exam->y==yGoal)
		{
			//on inverse la liste, pour que Open corresponde à la case de départ et non d'arrivée
			Noeud * New=NULL, * Temp2=NULL, * Suppr=NULL;

			Temp=Perso->Open;
			while(Temp!=NULL)
			{
				New=(Noeud *)malloc(sizeof(Noeud));
				New->x=Temp->x;
				New->y=Temp->y;
				if (Temp2!=NULL) Temp2->Precedent=New;
				New->Suivant=Temp2;
				New->Precedent=NULL;
				Suppr=Temp;
				Temp=Temp->Parent;
				free(Suppr);	//supression du noeud de l'ancienne liste
				Temp2=New;
			}
			Perso->Open=New;

			//calcul du premier noeud vers lequel se diriger
			Perso->Cible=Perso->Open;

			//le rayon d'accroche augmente (suivi du monstre)
			Perso->Approche=Perso->Approche_Origine*2;
			return 1;
		}
		else
		{
			//on détache Exam de Open
			if (Exam->Precedent==NULL)
			{
				Perso->Open=Exam->Suivant;
				if (Exam->Suivant!=NULL)	Perso->Open->Precedent=NULL;
			}
			else
			{
				Temp=Perso->Open;
				while(Temp!=NULL)
				{
					if (Temp->x==Exam->x && Temp->y==Exam->y)
					{
						Temp->Precedent->Suivant=Temp->Suivant;
						if (Temp->Suivant!=NULL)	Temp->Suivant->Precedent=Temp->Precedent;
						break;
					}
					Temp=Temp->Suivant;
				}
			}

			Examiner_Voisin(Perso,Exam,Exam->x-1,Exam->y,xGoal,yGoal);	//voisin de gauche
			Examiner_Voisin(Perso,Exam,Exam->x+1,Exam->y,xGoal,yGoal);	//voisin de droite
			Examiner_Voisin(Perso,Exam,Exam->x,Exam->y-1,xGoal,yGoal);	//voisin du haut
			Examiner_Voisin(Perso,Exam,Exam->x,Exam->y+1,xGoal,yGoal);	//voisin du bas
		}

		//on ajoute Exam à la fin de Closed
		if (Perso->Closed == NULL)	Perso->Closed=Exam;
		else
		{
			Temp=Perso->Closed;
			while (Temp->Suivant!=NULL)	Temp=Temp->Suivant;
			Exam->Precedent=Temp;
			Exam->Suivant=NULL;
			Temp->Suivant=Exam;
		}
	}
	//aucun chemin trouvé
	return 0;
}



/*
=================
Calculer_Chemin_Plus_Court

Calcul du chemin le plus coup pour arriver a la cible
=================
*/
void Calculer_Chemin_Plus_Court(character_t * Perso, character_t * Cible)
{
	//fonction lançant le PathFinder sur la recherche de Cible

//	Perso->Dernier_Calcul_PathFinding=game_getTime();

	unsigned int x_case1,y_case1,	//coordonnées de la case du monstre
				 x_case2,y_case2;	//coordonnées de la case de la cible

	//on détermine les cases où se trouvent les persos
	x_case1=Perso->x/16;
	y_case1=Perso->y/16;
	x_case2=(Cible->x)/16;
	y_case2=(Cible->y)/16;

	//on efface le dernier chemin trouvé
	Perso->Open=NULL;	
	PATHFINDER_NB_NOEUDS=0;

	//on lance la recherche du chemin le plus court
	AStar_Search(Perso,x_case1,y_case1,x_case2,y_case2);

	//pour le debug on sauvegarde dans un fichier texte
	if (SAVE_NOEUDS_DEBUG)
	{
		char s1[85];
		sprintf(s1,"%d\t%d\t\t%d\t%d\t\t%d",x_case1,y_case1,x_case2,y_case2,PATHFINDER_NB_NOEUDS);
		AddLog(s1);
	}
}





/*
=================
Afficher_Chemin_PathFinder

Afficher le chemin du pathfinder
=================
*/
void Afficher_Chemin_PathFinder(character_t *character)
{
	//si pas de noeud alors rien a afficher
	if (character->Open==NULL)	return;

	int x_case_ecran,y_case_ecran,	//coordonnées de la case haut gauche de la zone à afficher
		x_true_ecran,y_true_ecran,	//décalage de l'écran par rapport aux cases
		x,y;						//coordonnées d'affichage du sprite
	Noeud * Temp;

	//Temp pointe sur le noeud d'arrivée
	Temp=character->Open;	


	//calcul des possitions ICI on et sur des cases de 16x16
	x_case_ecran= character->x/16,
	y_case_ecran= character->y/16,
	x_true_ecran= -character->x+(16*x_case_ecran),
	y_true_ecran= -character->y+(16*y_case_ecran);

	//Tent que le noeud et pas totalement parcourue on continue
	while (Temp!=NULL)
	{
		x=16*(Temp->x-x_case_ecran)+x_true_ecran + character->x;
		y=16*(Temp->y-y_case_ecran)+y_true_ecran + character->y;


		if (Temp!=character->Cible)
			oslDrawRect(x,y , x+16,y+16 , RGBA(0, 0, 255, 255));
		else	
			oslDrawRect(x,y , x+16,y+16 , RGBA(255, 0, 255, 255));

	//passage au noeud parent
		Temp=Temp->Suivant;
	}
}





/*
=================
rand_ex

generer un nombre aleatoire compris entre mini et maxi
=================
*/
int rand_ex (int min, int max)
{
    int tmp;

     if (min == max) 
        return min; 

     if (min > max)
     {
        tmp = min;
        min = max;
        max = tmp;
     }

     return ( rand() % (max - min) ) + min;
}



/*
=================
main

Boucle principal du jeux
=================
*/
int main(int argc, char* argv[])
{
	//Initialisation OSLib
	oslInit (0);	
	oslInitGfx (OSL_PF_8888, 1);
	oslSetQuitOnLoadFailure (1);
	oslInitConsole();	

	oslSetBkColor(RGBA(0,0,0,0));

	//Initialisation des personnages
	Perso1 = character_new();
	Perso2 = character_new();

	//possition des personnages
	character_setPoss(Perso1,32,48);
	character_setPoss(Perso2,320,192);


	int tmp,tmp2,tmp3;

	//initialise le tableau pour les test
	Walkability=(int *)malloc(510*sizeof(int));	//30*17 cases
	//matrice d'exmple pour la gestion des collisions initialiser a 0 par defaut
	for (tmp=0; tmp < 30; tmp +=1)
	{
			for (tmp2=0; tmp2 < 17; tmp2 +=1)
			{	
				Walkability[30*tmp2+tmp]= 0;
			}

	}

    while (!osl_quit)
    {
	//preraration d'usage qu'il ne faut plus presenter
	oslStartDrawing();
	oslClearScreen(RGB(255,255,255));
	oslReadKeys();

	//deplacement du joueur
	if (osl_keys->pressed.up)
	{
		//verifi d'abord si il y a pas une collision
		if (!Walkability[30*(Perso2->y-16)/16 + Perso2->x/16])
		Perso2->y -=16;
	}
	if (osl_keys->pressed.down)
	{
		if (!Walkability[30*(Perso2->y+16)/16 + Perso2->x/16])
		Perso2->y +=16;
	}
	if (osl_keys->pressed.left)
	{
		if (!Walkability[30*Perso2->y/16 + (Perso2->x-16)/16])
		Perso2->x -=16;
	}
	if (osl_keys->pressed.right)
	{
		if (!Walkability[30*Perso2->y/16 + (Perso2->x+16)/16])
		Perso2->x +=16;
	}

	if (osl_keys->pressed.cross)
	{
		//change la valeur de la case ou ce situe Perso2 par une collision sur la carte
		Walkability[30*Perso2->y/16 + Perso2->x/16]=1;

		//verifie autour si on peut deplacer le joueur pour ne pas le laisser sur une case de collision
		if (!Walkability[30*(Perso2->y+16)/16 + Perso2->x/16])
		Perso2->y +=16;
		else if (!Walkability[30*Perso2->y/16 + (Perso2->x-16)/16])
		Perso2->x -=16;
		else if (!Walkability[30*Perso2->y/16 + (Perso2->x-16)/16])
		Perso2->x -=16;
		else if (!Walkability[30*Perso2->y/16 + (Perso2->x+16)/16])
		Perso2->x +=16;

	}

	//affichage des collisions en parcourant les X et Y
	for (tmp=0; tmp < 30; tmp +=1)
	{
			for (tmp2=0; tmp2 < 17; tmp2 +=1)
			{	
				if (Walkability[30*tmp2 + tmp])oslDrawFillRect(tmp*16, tmp2*16, tmp*16 + 16,  tmp2*16 +16, RGB(0,0,0));
			}

	}

	
	//trace la grille de fond pour l'exemple les Y
	for (tmp=0; tmp < 272; tmp +=16)
	{
		oslDrawLine(0,tmp,480,tmp, RGB(0,0,0));
	}
	//trace la grille de fond pour l'exemple les X
	for (tmp=0; tmp < 480; tmp +=16)
	{
		oslDrawLine(tmp,0,tmp,272, RGB(0,0,0));
	}


	//dessine les perso
	oslDrawFillRect(Perso1->x, Perso1->y, Perso1->x + 16,  Perso1->y+16, RGB(255,0,0));
	oslDrawFillRect(Perso2->x, Perso2->y, Perso2->x + 16,  Perso2->y+16, RGB(0,255,0));


	//pour le coup le calcul ce fait systematiquement, mais il faut mettre en place un timer pour limiter le nombre de recherche pour limiter le cout cpu pour la recherche
	Calculer_Chemin_Plus_Court(Perso1,Perso2);

	//affichage du chemin calculé par le PathFinder
	Afficher_Chemin_PathFinder(Perso1);



	//Fin du dessin
	oslEndDrawing();
	//Synchronise l'écran
	oslSyncFrame();		
    }

    oslEndGfx();
    oslQuit ();

	return 0;
}



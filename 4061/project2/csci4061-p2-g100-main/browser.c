/*
CSCI-4061 Fall 2022 - Project #2
* Group Member #1: Tor Anderbeck and06339
* Group Member #2: Elvir Semic semic002 <left before final submission>
* Group Member #3: Erik Heen heenx008
*/

#include "wrapper.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <signal.h>

#define MAX_TABS 100  // this gives us 99 tabs, 0 is reserved for the controller
// #define MAX_BAD 1000 // deprecated
#define MAX_URL 100
#define MAX_FAV 100
#define MAX_LABELS 100


comm_channel comm[MAX_TABS];         // Communication pipes 
char favorites[MAX_FAV][MAX_URL];     // Favorites list
int num_fav = 0;                     // # favorites

typedef struct tab_list {
  int free; // Boolean 1=T 0=F value
  pid_t pid; // may or may not be useful
} tab_list;

// Tab bookkeeping
tab_list TABS[MAX_TABS];
int TABS_QTY = 0; // # of tabs

/************************/
/* Simple tab functions */
/************************/

// return total number of tabs currently open
int get_num_tabs () {
  TABS_QTY = 1; // Tab #1 is always reserved for controller
	for (int i=0; i<MAX_TABS; i++) {
		if(TABS[i].free == 0) {
			TABS_QTY++;
		}
	}
  return TABS_QTY;
}


// Returns next free tab index, or -1 if there are none
int get_free_tab () {
	for (int i = 0; i < MAX_TABS; i++) {
		if(TABS[i].free == 1) {
			TABS[i].free = 0;
			return i;
		}
	}
  return -1;
}

// init TABS data structure
void init_tabs () {
  TABS[0].free = 0; // Used as controller's private pipe
  for (int i=1; i<MAX_TABS; i++) {
    TABS[i].free = 1; // Initialize every pipe to be considered free
    TABS[i].pid = -1; // Dummy value; test for this
  }
}

/***********************************/
/* Favorite manipulation functions */
/***********************************/

// Returns current size of Favorites list
int get_num_favorites () {
  num_fav = 0;
	for (int i = 0; i < MAX_FAV; i++) {
		if(strcmp(&favorites[i][0], "")) {
			num_fav++;
		}
	}
  return num_fav;
}

// return  0 if favorite is ok,
// return -1 if both max limit OR already a favorite
int fav_ok (char *uri) {
	if (on_favorites(uri) == 1) {
		alert("FAVORITE EXISTS");
		return -1;
	}
	else if (get_num_favorites() > MAX_FAV) {
		alert("EXCEEDED MAXIMUM FAVORITES");
		return -1;
	}
  return 0;
}



// Add uri to favorites file and update favorites array with the new favorite
void update_favorites_file (char *uri) {
  // Checks MAX_FAV first
  int nextFav = get_num_favorites();
  if(nextFav >= MAX_FAV) {
    perror("Favorites list is full! Favorite was not added.");
    return;
  } else if (strlen(uri) >= MAX_URL) { // >= not >, to account for the null byte
    perror("Favorite URL too long! Favorite was not added.");
    return;
  }

  // Add uri to favorites file
	FILE *uri_fd = fopen(".favorites", "a");
	if (uri_fd == NULL) {
		perror("Error opening favorites file\n");
		return;
	}
	fwrite(uri, MAX_URL, 1, uri_fd);
  if(ferror(uri_fd)) {
    perror("Error writing to favorites file\n");
  }
  fclose(uri_fd);

  // Update favorites array with the new favorite
  strcpy(&favorites[nextFav], uri);
  
  return;
}

// Set up favorites array
void init_favorites (char *fname) {
  // Open .favorites file
  FILE *fav_fd = fopen(fname, "r");
  if (fav_fd == NULL) {
    perror("Error opening favorites file\n");
    exit(1); 
    // This is only called by the controller, so if this fails then just
    // bail out because this program doesn't have good error handling
  }

  // Read in Favs and add to favorites
  // Because favorites is a global variable, its entries must be malloc'd
  // in order to remain in existence. See free_favorites() for the helper
  // function to clean them up.
  char *fURL_in = malloc(sizeof(char) * MAX_URL);
  if (fURL_in == NULL) {
    perror("malloc failed");
    fclose(fav_fd);
    exit(1);
  }
  for (int isEOF=fscanf(fav_fd,"%s",fURL_in); isEOF != EOF; isEOF=fscanf(fav_fd,"%s",fURL_in)) {
    char *fURL = malloc(sizeof(char) * MAX_URL);
    if (fURL == NULL) {
      perror("malloc failed");
      fclose(fav_fd);
      exit(1);
   }
    //FAV_LIST[FAV_QTY] = fURL;
    strncpy(favorites[num_fav], fURL, MAX_URL);
    num_fav++;
  }

  // Error checking & Cleanup
  if(ferror(fav_fd)) {
    perror("Error reading from Favorites file");
    fclose(fav_fd);
    exit(1);
  }

  fclose(fav_fd);

  return;
}

void free_favorites () {
  while(num_fav > 0) {
    free(favorites[num_fav]);
    num_fav--;
  }
  return;
}

// Make fd non-blocking just as in class!
// Return 0 if ok, -1 otherwise
// Really a util but I want you to do it :-)
int non_block_pipe (int fd) {
	int nFlags = fcntl(fd, F_GETFL, 0);
  if (nFlags < 0) { // If fcntl errored
    return -1;
  }
  if (fcntl(fd, F_SETFL, nFlags | O_NONBLOCK) < 0) {// If fcntl errored
    return -1;
  }
  // all is well, fd is now non-blocking
  return 0;
}

/***********************************/
/* Functions involving commands    */
/***********************************/

// Checks if tab is bad and url violates constraints; if so, return.
// Otherwise, send NEW_URI_ENTERED command to the tab on inbound pipe
void handle_uri (char *uri, int tab_index) {
	printf("URL selected is %s\n", uri);
  if(strlen(uri) >= MAX_URL) { // check uri length
    alert("URL EXCEEDS MAX_URL LENGTH");
    return;
  } else if(bad_format(uri) == 1) { // check uri format
		alert("BAD FORMAT");
		return;
	}

	// send command to inbound pipe
	req_t request; // req_t(NEW_URI_ENTERED, tab_index, uri);
	request.type = NEW_URI_ENTERED;
	request.tab_index = tab_index;
	strncpy(request.uri, uri, MAX_URL);
  // No need to handle I/O open/close as comm is a global variable
	write(comm[tab_index].inbound[1], &request, sizeof(req_t));

  return;
}


// A URI has been typed in, and the associated tab index is determined
// If everything checks out, a NEW_URI_ENTERED command is sent (see Hint)
// Short function
void uri_entered_cb (GtkWidget* entry, gpointer data) {
  if(data == NULL) {	
    return;
  }
  // Get the tab (hint: wrapper.h)
  int get_tab = query_tab_id_for_request(entry, data);
  // Get the URL (hint: wrapper.h)
	char* get_url = get_entered_uri(entry);
  // Hint: now you are ready to handle_the_uri
	handle_uri(get_url, get_tab);

  return;
}
  

// Called when + tab is hit
// Check tab limit ... if ok then do some heavy lifting (see comments)
// Create new tab process with pipes
// Long function
void new_tab_created_cb (GtkButton *button, gpointer data) {
  
  if (data == NULL) {
    return;
  }

  // at tab limit?
	if (get_num_tabs() > MAX_TABS) {
		alert("Tab limit reached");
		return;
	}

  // Get a free tab
	int next = get_free_tab();

  // Create communication pipes for this tab
  if ((pipe(comm[next].outbound)) == -1) {
  		perror("Error piping new tab outbound");
 	}
 	
 	if ((pipe(comm[next].inbound)) == -1) {
  		perror("Error piping new tab inbound");
 	}

  // Make the read ends non-blocking 
  fcntl(comm[next].inbound[0], F_SETFL, fcntl(comm[next].inbound[0], F_GETFL) | O_NONBLOCK);
  non_block_pipe(comm[next].inbound[0]);
  fcntl(comm[next].outbound[0], F_SETFL, fcntl(comm[next].outbound[0], F_GETFL) | O_NONBLOCK);
  non_block_pipe(comm[next].outbound[0]);
  
  // fork and create new render tab
  // Note: render has different arguments now: tab_index, both pairs of pipe fd's
  // (inbound then outbound) -- this last argument will be 4 integers "a b c d"
  // Hint: stringify args
  pid_t pid = fork();
  if (pid == -1) {
  	perror("Error forking");
  }
  if (pid == 0) {
  	char tab_index[5];
  	char pipe_fds[20];
  	sprintf (tab_index, "%d", next);
		sprintf (pipe_fds, "%d %d %d %d", comm[next].inbound[0], comm[next].inbound[1], comm[next].outbound[0], comm[next].outbound[1]);
  	execl("./render", "render", tab_index, pipe_fds, (char *) NULL);
    perror("Error rendering new tab");
    exit(1);
 	 }

  // Controller parent just does some TABS bookkeeping
}

// This is called when a favorite is selected for rendering in a tab
// Hint: you will use handle_uri ...
// However: you will need to first add "https://" to the uri so it can be rendered
// as favorites strip this off for a nicer looking menu
// Short
void menu_item_selected_cb (GtkWidget *menu_item, gpointer data) {

  if (data == NULL) {
    return;
  }
  
  // Note: For simplicity, currently we assume that the label of the menu_item is a valid url
  // get basic uri
  char *basic_uri = (char *)gtk_menu_item_get_label(GTK_MENU_ITEM(menu_item));

  // append "https://" for rendering
  char uri[MAX_URL];
  sprintf(uri, "https://%s", basic_uri);

  // Get the tab (hint: wrapper.h)
	int get_tab = query_tab_id_for_request(menu_item, data);
	
  // Hint: now you are ready to handle_the_uri
  handle_uri (uri, get_tab);

  return;
}


// BIG CHANGE: the controller now runs an loop so it can check all pipes
// Long function
int run_control() {
  browser_window * b_window = NULL;
  int i, nRead;
  req_t req;

  //Create controller window
  create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb),
		 G_CALLBACK(uri_entered_cb), &b_window, comm[0]);

  // Create favorites menu
  create_browser_menu(&b_window, &favorites, num_fav);
  
  while (1) {
    process_single_gtk_event();

    // Read from all tab pipes including private pipe (index 0)
    // to handle commands:
    // PLEASE_DIE (controller should die, self-sent): send PLEASE_DIE to all tabs
    // From any tab:
    //    IS_FAV: add uri to favorite menu (Hint: see wrapper.h), and update .favorites
    //    TAB_IS_DEAD: tab has exited, what should you do?

    // Loop across all pipes from VALID tabs -- starting from 0
    for (i=0; i<MAX_TABS; i++) {
      if (TABS[i].free) continue;
      printf("%d\n", comm[i].outbound[0]);
      nRead = read(comm[i].outbound[0], &req, sizeof(req_t));

      // Check that nRead returned something before handling cases

      // Case 1: PLEASE_DIE

      // Case 2: TAB_IS_DEAD
	    
      // Case 3: IS_FAV
    }
    usleep(1000);
  }
  return 0;
}


int main(int argc, char **argv)
{
  if (argc != 1) {
    fprintf (stderr, "browser <no_args>\n");
    exit (0);
  }
  init_tabs ();
  // init blacklist (see util.h), and favorites (write this, see above)
	init_blacklist(".blacklist");
	init_favorites(".favorites");
	

  // Fork controller
  pid_t pid = fork();
  if (pid == -1) {
  	perror("Error forking");
  }
  if (pid == 0) {
  // Child creates a pipe for itself comm[0]
  	if ((pipe(comm[0].outbound)) == -1) {
  		perror("Error piping");
 	}
	else 
  // then calls run_control ()
  	run_control();
  }
  // Parent waits ...
	wait(NULL); 
}

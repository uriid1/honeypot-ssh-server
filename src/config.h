#define DEBUG 0           // Debug output to stdout 
#define LOGGING 0         // Write log
#define SUPPORT_SQLITE 1  // Support sqlite 3
#define WRITE_SQLITE 1    // Write log to db

// Default params
#define PORT "22"
#define PATH_ECDSA_KEY  "/etc/honeypot-ssh/ssh_host_ecdsa_key"
#define PATH_LOG        "/var/log/honeypot-ssh/honeypot.log "
#define PATH_DATABASE   "/var/lib/honeypot-ssh/honeypot.db"
#define PASSWORD_ATTEMPTS 3
#define PASSWORD_INPUT_TIMEOUT_MS 100

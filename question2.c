#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <microhttpd.h>

#define PORT 3000
#define POST_SIZE 1024

int main() {
    // Initialize MySQL connection
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return 1;
    }

    if (mysql_real_connect(conn, "localhost", "user", "password", "database", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // Initialize MHD daemon
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &http_handler, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "MHD_start_daemon() failed\n");
        mysql_close(conn);
        return 1;
    }

    printf("Listening on port %d\n", PORT);

    // Start main loop
    while (1) {
        // ... continue handling incoming requests in the MHD daemon ...
    }

    // Cleanup
    MHD_stop_daemon(daemon);
    mysql_close(conn);

    return 0;
}

int http_handler(void *cls, struct MHD_Connection *connection, const char *url,
                 const char *method, const char *version, const char *upload_data,
                 size_t *upload_data_size, void **con_cls) {
    if (strcmp(url, "/newpost") != 0 || strcmp(method, "POST") != 0) {
        return MHD_NO;  // Not a POST request to /newpost
    }

    // Read the incoming post data
    char post_data[POST_SIZE];
    memset(post_data, 0, POST_SIZE);
    size_t bytes_read = MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, "post_data", post_data, POST_SIZE);
    if (bytes_read == 0) {
        return MHD_NO;  // No post_data parameter in request
    }

    // Save the post data to the SQL database
    char sql_query[1024];
    snprintf(sql_query, sizeof(sql_query), "INSERT INTO posts (data) VALUES ('%s')", post_data);
    if (mysql_query(conn, sql_query) != 0) {
        fprintf(stderr, "mysql_query() failed: %s\n", mysql_error(conn));
        return MHD_NO;
    }

    // Return success response
    const char *response = "Post saved successfully\n";
    struct MHD_Response *mhd_response = MHD_create_response_from_buffer(strlen(response), (void *) response, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, mhd_response);
    MHD_destroy_response(mhd_response);

    return ret;
}

//
// Created by mikita on 25/09/23.
//

#ifndef V2IMPL_APP_LAYER_H
#define V2IMPL_APP_LAYER_H

int prepare_unix_sock(char *socket_upper);
int handle_unix_socket(int fd, char *buf, int length);

#endif //V2IMPL_APP_LAYER_H

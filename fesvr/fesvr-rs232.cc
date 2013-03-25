// See LICENSE for license details.

#include "htif_rs232.h"

int main(int argc, char** argv)
{
  std::vector<std::string> args(argv + 1, argv + argc);
  htif_rs232_t htif(args);
  return htif.run();
}

#if 0
int poll_tohost(int coreid, addr_t sig_addr, int sig_len)
{
  reg_t tohost;
  while ((tohost = htif->read_cr(coreid, 30)) == 0);
  if (tohost == 1)
  {
    if (sig_len != -1)
    {
      int chunk_size = 16;
      assert(sig_addr % chunk_size == 0);
      int sig_len_aligned = (sig_len + chunk_size - 1)/chunk_size*chunk_size;

      uint8_t* signature = new uint8_t[sig_len_aligned];
      htif->memif().read(sig_addr, sig_len, signature);
      for (int i = sig_len; i < sig_len_aligned; i++)
        signature[i] = 0;

      for (int i = 0; i < sig_len_aligned; i += chunk_size)
      {
        for (int j = chunk_size - 1; j >= 0; j--)
          printf("%02x", signature[i+j]);
        printf("\n");
      }
      delete [] signature;
    }
    else

    return 0;
  }

  return -1;
}
#endif

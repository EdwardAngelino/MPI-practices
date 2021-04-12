#include <iomanip>
#include <iostream>
#include <ostream>
#include <vector>

#include <mpi.h>

struct Evento {
  char* evento;
  unsigned char cant;
};

struct Traza
{
  char* nombre;
  Evento* eventos;
  unsigned int cantEventos;
  bool revisado;
  unsigned int idTraza;
};

void pack_plista(int incount, Traza* data, MPI_Comm comm,
                 std::vector<char> &buf)
{
  int pos = 0;
  buf.clear();
  int size;

  MPI_Pack_size(1, MPI_INT, comm, &size);
  buf.resize(pos+size);
  MPI_Pack(&incount, 1, MPI_INT, buf.data(), buf.size(), &pos, comm);

  for(int t = 0; t < incount; ++t)
  {
    MPI_Pack_size(2, MPI_UNSIGNED, comm, &size);
    buf.resize(pos+size);
    MPI_Pack(&data[t].cantEventos, 1, MPI_UNSIGNED,
             buf.data(), buf.size(), &pos, comm);
    MPI_Pack(&data[t].idTraza, 1, MPI_UNSIGNED,
             buf.data(), buf.size(), &pos, comm);

    MPI_Pack_size(1, MPI_UNSIGNED_CHAR, comm, &size);
    buf.resize(pos+size);
    { // MPI does not know about C++ bool
      unsigned char revisado = data[t].revisado;
      MPI_Pack(&revisado, 1, MPI_UNSIGNED_CHAR,
               buf.data(), buf.size(), &pos, comm);
    }

    // This interprets Traza::nombre as a character code, which is probably incorrect
    // However, that is unlikely to be a problem, unless you are in a
    // heterogeneous ASCII/EBCDIC environment
    MPI_Pack_size(data[t].cantEventos, MPI_CHAR, comm, &size);
    buf.resize(pos+size);
    MPI_Pack(data[t].nombre, data[t].cantEventos, MPI_CHAR,
             buf.data(), buf.size(), &pos, comm);

    for(unsigned int e = 0; e < data[t].cantEventos; ++e)
    {
      // send count (interpret as a number)
      MPI_Pack_size(1, MPI_UNSIGNED_CHAR, comm, &size);
      buf.resize(pos+size);
      MPI_Pack(&data[t].eventos[e].cant, 1, MPI_UNSIGNED_CHAR,
               buf.data(), buf.size(), &pos, comm);

      // send events (interpret as character codes)
      MPI_Pack_size(data[t].eventos[e].cant, MPI_CHAR, comm, &size);
      buf.resize(pos+size);
      MPI_Pack(data[t].eventos[e].evento, data[t].eventos[e].cant, MPI_CHAR,
               buf.data(), buf.size(), &pos, comm);
    }
  }
  buf.resize(pos);
}

void unpack_plista(int &outcount, Traza* &data, MPI_Comm comm,
                   // buf cannot be reference-to-const since MPI_Unpack takes
                   // pointer-to-nonconst
                   std::vector<char> &buf)
{
  int pos = 0;

  MPI_Unpack(buf.data(), buf.size(), &pos, &outcount, 1, MPI_INT, comm);

  data = new Traza[outcount];
  for(int t = 0; t < outcount; ++t)
  {
    MPI_Unpack(buf.data(), buf.size(), &pos,
               &data[t].cantEventos, 1, MPI_UNSIGNED, comm);
    MPI_Unpack(buf.data(), buf.size(), &pos,
               &data[t].idTraza, 1, MPI_UNSIGNED, comm);

    { // MPI does not know about C++ bool
      unsigned char revisado;
      MPI_Unpack(buf.data(), buf.size(), &pos,
                 &revisado, 1, MPI_UNSIGNED_CHAR, comm);
      data[t].revisado = revisado;
    }

    // This interprets Traza::nombre as a character code, which is probably incorrect
    // However, that is unlikely to be a problem, unless you are in a
    // heterogeneous ASCII/EBCDIC environment
    data[t].nombre = new char[data[t].cantEventos];
    MPI_Unpack(buf.data(), buf.size(), &pos,
               data[t].nombre, data[t].cantEventos, MPI_CHAR, comm);

    data[t].eventos = new Evento[data[t].cantEventos];
    for(unsigned int e = 0; e < data[t].cantEventos; ++e)
    {
      // receive count (interpret as a number)
      MPI_Unpack(buf.data(), buf.size(), &pos,
                 &data[t].eventos[e].cant, 1, MPI_UNSIGNED_CHAR, comm);

      // receive events (interpret as character codes)
      data[t].eventos[e].evento = new char[data[t].eventos[e].cant];
      MPI_Unpack(buf.data(), buf.size(), &pos,
                 data[t].eventos[e].evento, data[t].eventos[e].cant, MPI_CHAR,
                 comm);
    }
  }
}

void send_plista(int incount, Traza* data, int dest, int tag, MPI_Comm comm)
{
  std::vector<char> buf;
  pack_plista(incount, data, comm, buf);
  MPI_Send(buf.data(), buf.size(), MPI_PACKED, dest, tag, comm);
}

void recv_plista(int &outcount, Traza* &data,
                 int src, int tag, MPI_Comm comm)
{
  MPI_Status status;
  MPI_Probe(src, tag, comm, &status);
  int size;
  MPI_Get_count(&status, MPI_PACKED, &size);
  std::vector<char> buf(size);
  MPI_Recv(buf.data(), buf.size(), MPI_PACKED, src, tag, comm, &status);

  unpack_plista(outcount, data, comm, buf);
}

void make_test_data(int &count, Traza *&data) {
  count = 2;
  data = new Traza[2] {
    {
      new char[3] { char(0), char(1), char(3) },
      new Evento[3] {
        {
          new char[4] { 'a', 'b', 'c', 'd' },
          (unsigned char)4,
        },
        {
          new char[3] { 'e', 'f', 'g' },
          (unsigned char)3,
        },
        {
          new char[2] { 'h', 'i' },
          (unsigned char)2,
        },
      },
      3u,
      true,
      0u,
    },
    {
      new char[1] { char(4) },
      new Evento[1] {
        {
          new char[1] { 'j' },
          (unsigned char)1,
        },
      },
      1u,
      false,
      1u,
    },
  };
}

void print_data(std::ostream &out, int count, const Traza *data)
{
  for(int t = 0; t < count; ++t)
  {
    std::cout << "{\n"
              << "  nombre = { ";
    for(unsigned e = 0; e < data[t].cantEventos; ++e)
      std::cout << int(data[t].nombre[e]) << ", ";
    std::cout << "},\n"
              << "  eventos = {\n";
    for(unsigned e = 0; e < data[t].cantEventos; ++e)
    {
      std::cout << "    {\n"
                << "      evento = { ";
      for(int c = 0; c < data[t].eventos[e].cant; ++c)
        std::cout << "'" << data[t].eventos[e].evento[c] << "', ";
      std::cout << "},\n"
                << "      cant = " << int(data[t].eventos[e].cant) << ",\n"
                << "    },\n";
    }
    std::cout << "  },\n"
              << "  cantEventos = " << data[t].cantEventos << ",\n"
              << "  revisado = " << data[t].revisado << ",\n"
              << "  idTraza = " << data[t].idTraza << ",\n"
              << "}," << std::endl;
  }
}

int main(int argc, char **argv)
{
  int rank;
  Traza *plista = nullptr;
  int count = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(rank == 1) {
    make_test_data(count, plista);
    send_plista(count, plista, 0, 0, MPI_COMM_WORLD);
  }
  else if(rank == 0) {
    recv_plista(count, plista, 1, 0, MPI_COMM_WORLD);

    std::cout << std::boolalpha;
    print_data(std::cout, count, plista);
  }
  // don't bother freeing allocated memory, the process is ending anyway
  MPI_Finalize();
}

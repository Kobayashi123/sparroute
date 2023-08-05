#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <netinet/if_ether.h>

/**
 * ソケットの初期化
 */
int InitRawSocket(char *device, int promiscFlag, int ipOnly)
{
    struct ifreq ifreq;
    struct sockaddr_ll sa;
    int soc;

    if (ipOnly)
    {
        /* socket()でデータリンク層を扱うファイルディスクリプタを得る． */
        if ((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
        {
            perror("socket");
            return (-1);
        }
    }
    else
    {
        if ((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
        {
            perror("socket");
            return (-1);
        }
    }

    memset(&ifreq, 0, sizeof(struct ifreq));
    strncpy(ifreq.ifr_name, device, sizeof(ifreq.ifr_name) - 1);

    /* ネットワークインタフェース名に対応したインタフェースのインデックスを得る． */
    if (ioctl(soc, SIOCGIFINDEX, &ifreq) < 0)
    {
        perror("ioctl");
        close(soc);
        return (-1);
    }

    sa.sll_family = PF_PACKET;

    /* プロトコルファミリーをセットする． */
    if (ipOnly)
    {
        sa.sll_protocol = htons(ETH_P_IP);
    }
    else
    {
        sa.sll_protocol = htons(ETH_P_ALL);
    }

    /* インタフェースのインデックスをセットする． */
    sa.sll_ifindex = ifreq.ifr_ifindex;

    /* ソケットに情報をセットする． */
    if (bind(soc, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        perror("bind");
        close(soc);
        return (-1);
    }

    if (promiscFlag)
    {
        /* デバイスのフラグを取得する． */
        if (ioctl(soc, SIOCGIFFLAGS, &ifreq) < 0)
        {
            perror("ioctl");
            close(soc);
            return (-1);
        }

        /* ifr_flagsのIFF_PROMISCビットをONにする． */
        ifreq.ifr_flags = ifreq.ifr_flags | IFF_PROMISC;
        if (ioctl(soc, SIOCSIFFLAGS, &ifreq) < 0)
        {
            perror("ioctl");
            close(soc);
            return (-1);
        }
    }

    return (soc);
}

/**
 * MACアドレスを文字列形式に変換する．
 */
char *my_ether_ntoa_r(unsigned char *hwaddr, char *buf, socklen_t size)
{
    snprintf(buf, size, "%02x:%02x:%02x:%02x:%02x:%02x", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

    return (buf);
}

/**
 * Ethernetヘッダを表示する．
 */
int PrintEtherHeader(struct ether_header *eh, FILE *fp)
{
    char buf[80];

    fprintf(fp, "ether_header-----------------------------\n");
    fprintf(fp, "ether_dhost=%s\n", my_ether_ntoa_r(eh->ether_dhost, buf, sizeof(buf)));
    fprintf(fp, "ether_shost=%s\n", my_ether_ntoa_r(eh->ether_shost, buf, sizeof(buf)));
    fprintf(fp, "ether_type=%02X ", ntohs(eh->ether_type));

    switch (ntohs(eh->ether_type))
    {
    case ETH_P_IP:
        fprintf(fp, "(IP)\n");
        break;
    case ETH_P_IPV6:
        fprintf(fp, "(IPV6)\n");
        break;
    case ETH_P_ARP:
        fprintf(fp, "(ARP)\n");
        break;
    default:
        fprintf(fp, "(unknown)\n");
        break;
    }

    return (0);
}

void usage(void)
{
    fprintf(stderr,
            "Usage: ltest [OPTIONS] DEVICE_NAME\n"
            "       OPTIONS := { -v[ersion] | -h[elp] }\n");
    exit(-1);
}

int main(int argc, char *argv[], char *envp[])
{
    int soc, size;
    unsigned char buf[2048];

    while (argc > 1)
    {
        if (argv[1][0] != '-')
            break;
        else if (strcmp(argv[1], "-V") == 0)
        {
            printf("ltest-1.0.0\n");
            exit(0);
        }
        else if (strcmp(argv[1], "-help") == 0)
        {
            usage();
        }
        else
        {
            fprintf(stderr,
                    "Option \"%s\" is unknown, try \"ltest -help\".\n",
                    argv[1]);
            exit(-1);
        }
        argc--;
        argv++;
    }

    if (argc <= 1)
    {
        fprintf(stderr,
                "Usage: ltest [ DEVICE_NAME ]\n"
                "ex) ./ltest enp7s0\n");
        return (1);
    }

    if ((soc = InitRawSocket(argv[1], 0, 0)) == -1)
    {
        fprintf(stderr, "InitRawSocket:error: %s\n", argv[1]);
        return (-1);
    }

    /* read()でデータを受信し，Ethernetヘッダのサイズ以上受信した場合には，出力する． */
    while (1)
    {
        // struct sockaddr_ll from;
        // socklen_t fromLen;
        // memset(&from, 0, sizeof(from));

        // fromLen = sizeof(from);
        // if ((size = recvfrom(soc, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromLen)) <= 0)
        // {
        //     perror("recvfrom");
        // }
        // else
        // {
        //     printf("sll_family = %d\n", from.sll_family);     // PF_PACKET
        //     printf("sll_protocol = %d\n", from.sll_protocol); // ETH_P_IP
        //     printf("sll_ifindex = %d\n", from.sll_ifindex);   // デバイス番号
        //     printf("sll_hatype = %d\n", from.sll_hatype);     // ARPHRD_ETHER
        //     printf("sll_pkttype = %d\n", from.sll_pkttype);
        //     printf("sll_halen = %d\n", from.sll_halen);
        //     printf("sll_addr = %02x:%02x:%02x:%02x:%02x:%02x\n",
        //            from.sll_addr[0], from.sll_addr[1], from.sll_addr[2], from.sll_addr[3], from.sll_addr[4], from.sll_addr[5]);
        // }

        if ((size = read(soc, buf, sizeof(buf))) <= 0)
        {
            perror("read");
        }
        else
        {
            if (size >= sizeof(struct ether_header))
            {
                PrintEtherHeader((struct ether_header *)buf, stdout);
            }
            else
            {
                fprintf(stderr, "read size(%d) < %ld\n", size, sizeof(struct ether_header));
            }
        }
    }

    close(soc);

    return 0;
}

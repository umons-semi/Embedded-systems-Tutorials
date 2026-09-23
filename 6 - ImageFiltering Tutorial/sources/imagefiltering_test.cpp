#include <iostream>
#include <vector>
#include <cmath>

#include "imagefiltering.h"

int main()
{
    // Petite image artificielle 16 x 16 générée directement en mémoire.
    // Aucun fichier image et aucune bibliothèque OpenCV ne sont nécessaires.
    const int width = 16;
    const int height = 16;
    const int pixel_count = width * height;

    hls::stream<packet> s_in;
    hls::stream<packet> s_out;

    /*
     * Création d'une image avec une séparation horizontale :
     *
     * Partie supérieure : pixels à 0
     * Partie inférieure : pixels à 255
     *
     * Le filtre Sobel vertical utilisé plus bas doit détecter
     * la frontière entre ces deux zones.
     */
    std::vector<float> input_image(pixel_count, 0.0f);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (y >= height / 2)
                input_image[y * width + x] = 255.0f;
        }
    }

    // Envoi de l'image artificielle vers l'entrée AXI Stream.
    for (int i = 0; i < pixel_count; ++i)
    {
        packet in_packet;

        in_packet.data = input_image[i];
        in_packet.keep = -1;
        in_packet.strb = -1;
        in_packet.last = (i == pixel_count - 1);

        s_in.write(in_packet);
    }

    // Filtre Sobel vertical, identique au testbench original.
    float kernel[3 * 3] =
    {
        -1.0f, -2.0f, -1.0f,
         0.0f,  0.0f,  0.0f,
         1.0f,  2.0f,  1.0f
    };

    int test_width = width;
    int test_height = height;

    int return_value =
        imagefiltering_compute(s_in, s_out,
                               test_width, test_height,
                               kernel);

    if (return_value != 1)
    {
        std::cerr << "ERREUR : imagefiltering_compute a retourne "
                  << return_value << std::endl;
        return 1;
    }

    std::vector<float> output_image(pixel_count, 0.0f);

    bool last_detected = false;
    float maximum_absolute_value = 0.0f;

    // Lecture et vérification de la sortie.
    for (int i = 0; i < pixel_count; ++i)
    {
        if (s_out.empty())
        {
            std::cerr << "ERREUR : flux de sortie vide au pixel "
                      << i << std::endl;
            return 1;
        }

        packet out_packet;
        s_out.read(out_packet);

        output_image[i] = static_cast<float>(out_packet.data);

        float absolute_value = std::fabs(output_image[i]);
        if (absolute_value > maximum_absolute_value)
            maximum_absolute_value = absolute_value;

        if (out_packet.last)
        {
            if (i != pixel_count - 1)
            {
                std::cerr << "ERREUR : TLAST recu trop tot au pixel "
                          << i << std::endl;
                return 1;
            }

            last_detected = true;
        }
    }

    if (!last_detected)
    {
        std::cerr << "ERREUR : TLAST absent sur le dernier pixel."
                  << std::endl;
        return 1;
    }

    if (!s_out.empty())
    {
        std::cerr << "ERREUR : le flux contient plus de "
                  << pixel_count << " pixels." << std::endl;
        return 1;
    }

    /*
     * Affichage de la sortie sous forme de matrice.
     * Cela permet de repérer les valeurs fortes autour de la frontière.
     */
    std::cout << "\nImage d'entree (" << width << " x "
              << height << ") :" << std::endl;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
            std::cout << input_image[y * width + x] << "\t";

        std::cout << std::endl;
    }

    std::cout << "\nSortie du filtre :" << std::endl;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
            std::cout << output_image[y * width + x] << "\t";

        std::cout << std::endl;
    }

    std::cout << "\nValeur absolue maximale : "
              << maximum_absolute_value << std::endl;

    if (maximum_absolute_value == 0.0f)
    {
        std::cerr << "ERREUR : le filtre n'a detecte aucune variation."
                  << std::endl;
        return 1;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "C SIMULATION REUSSIE" << std::endl;
    std::cout << "Nombre de pixels recus : " << pixel_count << std::endl;
    std::cout << "TLAST correctement detecte." << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
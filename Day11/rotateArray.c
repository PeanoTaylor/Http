#include <stdio.h>

void rotateLeft(int arr[], int n, int k)
{
    if (n < 0)
    {
        return;
    }
    k = k % n;
    int temp[n];
    for (int i = 0; i < n; ++i)
    {
        temp[i] = arr[(i + k) % n];
    }

    for (int i = 0; i < n; ++i)
    {
        arr[i] = temp[i];
    }
}

int main()
{
    int arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    int n = sizeof(arr) / sizeof(arr[0]);
    rotateLeft(arr, n, 3);
    for (int i = 0; i < n; ++i)
    {
        printf("%d ", arr[i]);
    }
    return 0;
}

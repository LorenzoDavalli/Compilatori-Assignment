void fun(){

        int v[20];

        for (int i = 0; i < 15; i++){
                v[i] = 1;

        }
        //Verificare che hanno lo stesso rray di base


        for (int i = 0; i < 15; i++){
                v[i] = v[i+5];
        }
}

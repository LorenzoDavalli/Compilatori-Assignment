int example(int n, int a, int b) {
    int sum = 0;
    int i;

    while( 0 == 0 ) {
        i = i + 1;
        // Istruzioni LOOP-INVARIANT (LI) - Potrebbero essere spostate fuori dal ciclo
        int li1 = a * b;         // LI1: Dipende solo da 'a' e 'b' (costanti nel ciclo)
        int li2 = a + 10;        // LI2: Dipende solo da 'a'

        // Istruzioni LOOP-VARIANT (LV) - Devono rimanere nel ciclo
        int lv1 = i * 2;         // LV1: Dipende da 'i'
        int lv2 = sum + i;       // LV2: Dipende da 'i' e 'sum' (accumulatore)

        sum += li1 + li2 + lv1 + lv2;
        if( i == 100) { break; }



        }
        
        
         while( 0 == 0 ) {
		i = i + 1;
		// Istruzioni LOOP-INVARIANT (LI) - Potrebbero essere spostate fuori dal ciclo
		int li1 = a * b;         // LI1: Dipende solo da 'a' e 'b' (costanti nel ciclo)
		int li2 = a + 10;        // LI2: Dipende solo da 'a'

		// Istruzioni LOOP-VARIANT (LV) - Devono rimanere nel ciclo
		int lv1 = i * 2;         // LV1: Dipende da 'i'
		int lv2 = sum + i;       // LV2: Dipende da 'i' e 'sum' (accumulatore)

		sum += li1 + li2 + lv1 + lv2;
		if( i == 100) { break; }



        }

        return sum;
}

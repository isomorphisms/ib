package org.isomorphisms.ib.prepaint;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

import org.junit.Test;

public final class TokenizerExperimentTest {
    @Test
    public void exactOracleKeepsQuotesEntitiesAndMalformedRecoveryDistinct() {
        TokenizerExperiment.Result result = TokenizerExperiment.scalar(bytes(
                "alpha <a title=\"x > y &amp; z\" q='u < v'>tail &broken <b>"));

        assertEquals(List.of(
                "RUN@0/DATA:alpha ",
                "TAG_OPEN@6/DATA:<",
                "RUN@7/TAG:a title=",
                "DOUBLE_QUOTE_OPEN@15/TAG:\"",
                "RUN@16/DOUBLE_QUOTED:x > y ",
                "ENTITY@22/DOUBLE_QUOTED:&amp;",
                "RUN@27/DOUBLE_QUOTED: z",
                "DOUBLE_QUOTE_CLOSE@29/DOUBLE_QUOTED:\"",
                "RUN@30/TAG: q=",
                "SINGLE_QUOTE_OPEN@33/TAG:'",
                "RUN@34/SINGLE_QUOTED:u < v",
                "SINGLE_QUOTE_CLOSE@39/SINGLE_QUOTED:'",
                "TAG_CLOSE@40/TAG:>",
                "RUN@41/DATA:tail ",
                "MALFORMED_ENTITY@46/DATA:&broken",
                "RUN@53/DATA: ",
                "TAG_OPEN@54/DATA:<",
                "RUN@55/TAG:b",
                "TAG_CLOSE@56/TAG:>"), render(result.events));
    }

    @Test
    public void candidateMatchesOracleAcrossEverySingleChunkBoundary() {
        List<byte[]> fixtures = List.of(
                bytes("ordinary text with a long boring run before <b>markup</b>"),
                bytes("<a title=\"x > y &amp; z\" q='u < v'>tail</a>"),
                bytes("A &broken <b x='unterminated"),
                bytes("zero\0byte<tag a=&bad>z"),
                bytes("pi=π; 猫 &amp; dog"));

        for (byte[] fixture : fixtures) {
            List<TokenizerExperiment.Event> expected = TokenizerExperiment.scalar(fixture).events;
            assertEquals(expected, TokenizerExperiment.tableDriven(fixture).events);

            for (int split = 0; split <= fixture.length; split++) {
                List<byte[]> chunks = List.of(
                        Arrays.copyOfRange(fixture, 0, split),
                        Arrays.copyOfRange(fixture, split, fixture.length));
                assertEquals(expected, TokenizerExperiment.scalar(chunks).events);
                assertEquals(expected, TokenizerExperiment.tableDriven(chunks).events);
            }

            List<byte[]> oneByteChunks = new ArrayList<>();
            for (byte value : fixture) {
                oneByteChunks.add(new byte[] {value});
            }
            assertEquals(expected, TokenizerExperiment.scalar(oneByteChunks).events);
            assertEquals(expected, TokenizerExperiment.tableDriven(oneByteChunks).events);
        }
    }

    @Test
    public void utf8IsAnExactRawByteHandoffEvenWhenAChunkSplitsInsideASequence() {
        byte[] fixture = bytes("Aπ猫B");
        TokenizerExperiment.Result expected = TokenizerExperiment.scalar(fixture);
        List<TokenizerExperiment.Event> utf8 = expected.events.stream()
                .filter(event -> event.kind == TokenizerExperiment.EventKind.UTF8_HANDOFF)
                .collect(Collectors.toList());

        assertEquals(1, utf8.size());
        assertArrayEquals(bytes("π猫"), utf8.get(0).bytes);
        assertEquals(1, utf8.get(0).offset);

        for (int split = 1; split < fixture.length; split++) {
            List<byte[]> chunks = List.of(
                    Arrays.copyOfRange(fixture, 0, split),
                    Arrays.copyOfRange(fixture, split, fixture.length));
            assertEquals(expected.events, TokenizerExperiment.tableDriven(chunks).events);
        }
    }

    @Test
    public void malformedEntityNulAndUnclosedQuoteStayVisibleToTheOracle() {
        TokenizerExperiment.Result result = TokenizerExperiment.scalar(bytes(
                "x &broken <a q='unterminated\0"));

        assertEquals(List.of(
                "RUN@0/DATA:x ",
                "MALFORMED_ENTITY@2/DATA:&broken",
                "RUN@9/DATA: ",
                "TAG_OPEN@10/DATA:<",
                "RUN@11/TAG:a q=",
                "SINGLE_QUOTE_OPEN@15/TAG:'",
                "RUN@16/SINGLE_QUOTED:unterminated",
                "NUL@28/SINGLE_QUOTED:\\x00",
                "MALFORMED_EOF@29/SINGLE_QUOTED:"), render(result.events));
    }

    @Test
    public void longOrdinaryRunActuallyUsesTheBulkPathWithoutSimd() {
        byte[] fixture = bytes("x".repeat(4096) + "<a q=\"y\">z</a>");
        TokenizerExperiment.Result scalar = TokenizerExperiment.scalar(fixture);
        TokenizerExperiment.Result candidate = TokenizerExperiment.tableDriven(fixture);

        assertEquals(scalar.events, candidate.events);
        assertTrue(candidate.bulkBytes >= 4096);
        assertTrue(candidate.exceptionalDispatches < 32);
        assertEquals(fixture.length, scalar.exceptionalDispatches);
    }

    private static byte[] bytes(String text) {
        return text.getBytes(StandardCharsets.UTF_8);
    }

    private static List<String> render(List<TokenizerExperiment.Event> events) {
        return events.stream().map(Object::toString).collect(Collectors.toList());
    }
}

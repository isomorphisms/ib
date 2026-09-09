package org.isomorphisms.ib.material3

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.weight
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                ArxivViewport()
            }
        }
    }
}

private data class FrozenArticle(
    val key: String,
    val title: String,
    val authors: String,
    val sourceLabel: String,
    val body: String,
)

/*
 * These are deliberately fixtures, not invented arXiv records.  Replace them with
 * the five frozen Cauldron articles once that corpus is wired into this prototype.
 */
private val frozenArticles = List(5) { index ->
    val number = index + 1
    FrozenArticle(
        key = "frozen-$number",
        title = "Frozen arXiv article $number",
        authors = "authors from frozen source",
        sourceLabel = "Cauldron fixture $number",
        body = """
            This is the text viewport for frozen arXiv article $number.

            The first prototype intentionally proves only two things: a five-item
            lazy article chooser and a readable, selectable, independently
            scrollable text surface for the selected article.

            The article body will come from IB's frozen pre-paint/Cauldron data.
            This view does not fetch the network and does not parse HTML.

            Individual page pieces are intentionally not draggable yet.  Dragging
            is planned as a later interaction layer so it does not contaminate the
            article, text, or list data model.
        """.trimIndent(),
    )
}

@Composable
private fun ArxivViewport() {
    var selected by remember { mutableStateOf(frozenArticles.first()) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("IB") },
            )
        },
    ) { scaffoldPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(scaffoldPadding),
        ) {
            Text(
                text = "Cauldron · five frozen arXiv articles",
                style = MaterialTheme.typography.labelLarge,
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
            )

            LazyColumn(
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(max = 260.dp),
            ) {
                items(
                    items = frozenArticles,
                    key = { article -> article.key },
                ) { article ->
                    ListItem(
                        headlineContent = { Text(article.title) },
                        supportingContent = {
                            Text("${article.authors} · ${article.sourceLabel}")
                        },
                        modifier = Modifier.clickable { selected = article },
                    )
                    HorizontalDivider()
                }
            }

            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f)
                    .padding(12.dp),
                shape = MaterialTheme.shapes.large,
                tonalElevation = 1.dp,
            ) {
                SelectionContainer {
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .verticalScroll(rememberScrollState())
                            .padding(18.dp),
                    ) {
                        Text(
                            text = selected.title,
                            style = MaterialTheme.typography.headlineSmall,
                        )
                        Text(
                            text = selected.authors,
                            style = MaterialTheme.typography.bodyMedium,
                        )
                        Text(
                            text = selected.sourceLabel,
                            style = MaterialTheme.typography.labelMedium,
                        )
                        Spacer(modifier = Modifier.height(18.dp))
                        Text(
                            text = selected.body,
                            style = MaterialTheme.typography.bodyLarge,
                        )
                    }
                }
            }
        }
    }
}
